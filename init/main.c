#define __LIBRARY__
#include <unistd.h>
#include <time.h>

/*
 * 我们需要这个内联函数 - 从内核空间fork将导致
 * 没有写时复制（!!!），直到执行execve为止。
 * 这不是问题，但是对于堆栈来说是个问题。
 * 这通过让main()在fork()之后根本不使用堆栈来处理。
 * 因此，没有函数调用 - 这意味着fork也需要内联代码，
 * 否则我们在退出'fork()'时会使用堆栈。
 *
 * 实际上只有pause和fork需要内联，这样main()就不会
 * 对堆栈进行任何操作，但我们定义了一些其他的内联函数。
 */
static inline _syscall0(int,fork)	/* fork系统调用 */
static inline _syscall0(int,pause)	/* pause系统调用 */
static inline _syscall0(int,setup)	/* setup系统调用 */
static inline _syscall0(int,sync)	/* sync系统调用 */

#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <asm/system.h>
#include <asm/io.h>

#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <linux/fs.h>

static char printbuf[1024];	/* 打印缓冲区 */

extern int vsprintf();		/* 格式化字符串打印函数 */
extern void init(void);		/* 初始化函数 */
extern void hd_init(void);	/* 硬盘初始化函数 */
extern long kernel_mktime(struct tm * tm);	/* 内核时间设置函数 */
extern long startup_time;	/* 系统启动时间 */

/*
 * 是的，是的，这很丑陋，但我无法找到正确的方法来做到这一点
 * 这似乎有效。如果有人对实时时钟有更多信息，我会很感兴趣。
 * 这大部分是试验和错误，以及一些bios列表阅读。呃。
 */

/* 读取CMOS寄存器 */
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

/* BCD码转换为二进制 */
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

/* 时间初始化函数 */
static void time_init(void)
{
	struct tm time;	/* 时间结构体 */

	do {
		time.tm_sec = CMOS_READ(0);	/* 读取秒 */
		time.tm_min = CMOS_READ(2);	/* 读取分 */
		time.tm_hour = CMOS_READ(4);	/* 读取时 */
		time.tm_mday = CMOS_READ(7);	/* 读取日 */
		time.tm_mon = CMOS_READ(8)-1;	/* 读取月 */
		time.tm_year = CMOS_READ(9);	/* 读取年 */
	} while (time.tm_sec != CMOS_READ(0));	/* 确保时间读取一致 */
	BCD_TO_BIN(time.tm_sec);	/* 转换秒 */
	BCD_TO_BIN(time.tm_min);	/* 转换分 */
	BCD_TO_BIN(time.tm_hour);	/* 转换时 */
	BCD_TO_BIN(time.tm_mday);	/* 转换日 */
	BCD_TO_BIN(time.tm_mon);	/* 转换月 */
	BCD_TO_BIN(time.tm_year);	/* 转换年 */
	startup_time = kernel_mktime(&time);	/* 设置启动时间 */
}

/* 内核主函数 */
void main(void)		/* 这确实是void，这里没有错误 */
{			/* 启动例程假设（好吧，...）这一点 */
/*
 * 中断仍然被禁用。进行必要的设置，然后启用它们
 */
	time_init();	/* 时间初始化 */
	tty_init();	/* 终端初始化 */
	trap_init();	/* 异常处理初始化 */
	sched_init();	/* 调度器初始化 */
	buffer_init();	/* 缓冲区初始化 */
	hd_init();	/* 硬盘初始化 */
	sti();		/* 开启中断 */
	move_to_user_mode();	/* 切换到用户模式 */
	if (!fork()) {		/* 我们指望这个能正常工作 */
		init();		/* 调用init函数 */
	}
/*
 *   注意！！对于任何其他任务，'pause()'意味着我们必须获得一个
 * 信号来唤醒，但task0是唯一的例外（参见'schedule()'）
 * 因为task0在每个空闲时刻都会被激活（当没有其他任务
 * 可以运行时）。对于task0，'pause()'只是意味着我们去检查是否有一些其他
 * 任务可以运行，如果没有，我们就返回到这里。
 */
	for(;;) pause();	/* 无限循环调用pause */
}

/* 格式化打印函数 */
static int printf(const char *fmt, ...)
{
	va_list args;	/* 可变参数列表 */
	int i;		/* 返回值 */

	va_start(args, fmt);	/* 初始化参数列表 */
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));	/* 写入标准输出 */
	va_end(args);	/* 清理参数列表 */
	return i;	/* 返回写入的字符数 */
}

/* 命令行参数和环境变量 */
static char * argv[] = { "-",NULL };
static char * envp[] = { "HOME=/usr/root", NULL };

/* 初始化函数 */
void init(void)
{
	int i,j;	/* 循环变量 */

	setup();	/* 系统设置 */
	if (!fork())	/* fork子进程 */
		_exit(execve("/bin/update",NULL,NULL));	/* 执行update程序 */
	(void) open("/dev/tty0",O_RDWR,0);	/* 打开终端设备 */
	(void) dup(0);	/* 复制文件描述符 */
	(void) dup(0);	/* 复制文件描述符 */
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);	/* 打印缓冲区信息 */
	printf(" Ok.\n\r");	/* 打印OK信息 */
	if ((i=fork())<0)	/* fork子进程 */
		printf("Fork failed in init\r\n");	/* fork失败 */
	else if (!i) {	/* 子进程 */
		close(0);close(1);close(2);	/* 关闭文件描述符 */
		setsid();	/* 创建新会话 */
		(void) open("/dev/tty0",O_RDWR,0);	/* 打开终端 */
		(void) dup(0);	/* 复制文件描述符 */
		(void) dup(0);	/* 复制文件描述符 */
		_exit(execve("/bin/sh",argv,envp));	/* 执行shell */
	}
	j=wait(&i);	/* 等待子进程 */
	printf("child %d died with code %04x\n",j,i);	/* 打印子进程退出信息 */
	sync();		/* 同步文件系统 */
	_exit(0);	/* 退出，注意是_exit，不是exit() */
}