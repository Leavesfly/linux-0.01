/*
 * 'sched.c'是主要的内核文件。它包含调度原语
 * (sleep_on, wakeup, schedule等)以及一些简单的系统
 * 调用函数(如getpid()，它只是从当前任务中提取字段)
 */
#include <linux/sched.h>
#include <linux/kernel.h>
#include <signal.h>
#include <linux/sys.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/segment.h>

#define LATCH (1193180/HZ)

extern void mem_use(void);

extern int timer_interrupt(void);
extern int system_call(void);

// 任务联合体，包含任务结构和栈空间
union task_union {
	struct task_struct task;  // 任务结构体
	char stack[PAGE_SIZE];    // 任务栈空间
};

// 初始化任务
static union task_union init_task = {INIT_TASK,};

// 全局变量：jiffies表示系统启动以来的时钟滴答数，startup_time表示系统启动时间
long volatile jiffies=0;
long startup_time=0;
// 当前运行任务指针和最后使用数学协处理器的任务指针
struct task_struct *current = &(init_task.task), *last_task_used_math = NULL;

// 任务数组，用于存储系统中的所有任务
struct task_struct * task[NR_TASKS] = {&(init_task.task), };

// 用户态栈空间
long user_stack [ PAGE_SIZE>>2 ] ;

// 栈起始结构，包含栈顶指针和代码段选择子
struct {
	long * a;
	short b;
	} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };
/*
 *  'math_state_restore()'保存当前的数学协处理器状态信息到
 * 旧的数学状态数组中，并从当前任务中获取新的状态信息
 */
void math_state_restore()
{
	if (last_task_used_math)
		__asm__("fnsave %0"::"m" (last_task_used_math->tss.i387));
	if (current->used_math)
		__asm__("frstor %0"::"m" (current->tss.i387));
	else {
		__asm__("fninit"::);
		current->used_math=1;
	}
	last_task_used_math=current;
}

/*
 *  'schedule()'是调度器函数。这是优秀的代码！
 * 在所有情况下都应该能良好工作（例如给IO密集型进程良好的响应等）。
 * 你可能需要关注的一点是这里的信号处理代码。
 *
 *   注意！！任务0是'空闲'任务，当没有其他任务可以运行时被调用。
 * 它不能被杀死，也不能睡眠。任务[0]中的'state'信息从未被使用。
 */
void schedule(void)
{
	int i,next,c;
	struct task_struct ** p;

/* 检查闹钟，唤醒任何收到信号的可中断任务 */

	for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
		if (*p) {
			if ((*p)->alarm && (*p)->alarm < jiffies) {
					(*p)->signal |= (1<<(SIGALRM-1));
					(*p)->alarm = 0;
				}
			if ((*p)->signal && (*p)->state==TASK_INTERRUPTIBLE)
				(*p)->state=TASK_RUNNING;
		}

/* 这是调度器的核心部分: */

	while (1) {
		c = -1;
		next = 0;
		i = NR_TASKS;
		p = &task[NR_TASKS];
		while (--i) {
			if (!*--p)
				continue;
			if ((*p)->state == TASK_RUNNING && (*p)->counter > c)
				c = (*p)->counter, next = i;
		}
		if (c) break;
		for(p = &LAST_TASK ; p > &FIRST_TASK ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) +
						(*p)->priority;
	}
	switch_to(next);
}

// 系统调用pause的实现，使当前进程进入可中断的睡眠状态
int sys_pause(void)
{
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	return 0;
}

// 使当前进程进入不可中断的睡眠状态
void sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp = *p;
	*p = current;
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();
	if (tmp)
		tmp->state=0;
}

// 使当前进程进入可中断的睡眠状态
void interruptible_sleep_on(struct task_struct **p)
{
	struct task_struct *tmp;

	if (!p)
		return;
	if (current == &(init_task.task))
		panic("task[0] trying to sleep");
	tmp=*p;
	*p=current;
repeat:	current->state = TASK_INTERRUPTIBLE;
	schedule();
	if (*p && *p != current) {
		(**p).state=0;
		goto repeat;
	}
	*p=NULL;
	if (tmp)
		tmp->state=0;
}

// 唤醒睡眠在指定等待队列上的进程
void wake_up(struct task_struct **p)
{
	if (p && *p) {
		(**p).state=0;
		*p=NULL;
	}
}

// 定时器处理函数，更新当前进程的时间统计
void do_timer(long cpl)
{
	if (cpl)
		current->utime++;
	else
		current->stime++;
	if ((--current->counter)>0) return;
	current->counter=0;
	if (!cpl) return;
	schedule();
}

// 设置闹钟信号
int sys_alarm(long seconds)
{
	current->alarm = (seconds>0)?(jiffies+HZ*seconds):0;
	return seconds;
}

// 获取当前进程ID
int sys_getpid(void)
{
	return current->pid;
}

// 获取父进程ID
int sys_getppid(void)
{
	return current->father;
}

// 获取用户ID
int sys_getuid(void)
{
	return current->uid;
}

// 获取有效用户ID
int sys_geteuid(void)
{
	return current->euid;
}

// 获取组ID
int sys_getgid(void)
{
	return current->gid;
}

// 获取有效组ID
int sys_getegid(void)
{
	return current->egid;
}

// 调整进程优先级
int sys_nice(long increment)
{
	if (current->priority-increment>0)
		current->priority -= increment;
	return 0;
}

// 设置信号处理函数
int sys_signal(long signal,long addr,long restorer)
{
	long i;

	switch (signal) {
		case SIGHUP: case SIGINT: case SIGQUIT: case SIGILL:
		case SIGTRAP: case SIGABRT: case SIGFPE: case SIGUSR1:
		case SIGSEGV: case SIGUSR2: case SIGPIPE: case SIGALRM:
		case SIGCHLD:
			i=(long) current->sig_fn[signal-1];
			current->sig_fn[signal-1] = (fn_ptr) addr;
			current->sig_restorer = (fn_ptr) restorer;
			return i;
		default: return -1;
	}
}

// 调度器初始化函数
void sched_init(void)
{
	int i;
	struct desc_struct * p;

	set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));
	set_ldt_desc(gdt+FIRST_LDT_ENTRY,&(init_task.task.ldt));
	p = gdt+2+FIRST_TSS_ENTRY;
	for(i=1;i<NR_TASKS;i++) {
		task[i] = NULL;
		p->a=p->b=0;
		p++;
		p->a=p->b=0;
		p++;
	}
	ltr(0);
	lldt(0);
	outb_p(0x36,0x43);		/* binary, mode 3, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	set_intr_gate(0x20,&timer_interrupt);
	outb(inb_p(0x21)&~0x01,0x21);
	set_system_gate(0x80,&system_call);
}