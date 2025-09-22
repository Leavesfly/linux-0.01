#ifndef _SCHED_H
#define _SCHED_H

/* 任务数量 */
#define NR_TASKS 64
/* 系统时钟频率（Hz） */
#define HZ 100

/* 第一个任务 */
#define FIRST_TASK task[0]
/* 最后一个任务 */
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>

/* 如果NR_OPEN大于32，则报错 */
#if (NR_OPEN > 32)
#error "Currently the close-on-exec-flags are in one word, max 32 files/proc"
#endif

/* 任务状态定义 */
#define TASK_RUNNING		0	/* 运行状态 */
#define TASK_INTERRUPTIBLE	1	/* 可中断睡眠状态 */
#define TASK_UNINTERRUPTIBLE	2	/* 不可中断睡眠状态 */
#define TASK_ZOMBIE		3	/* 僵尸状态 */
#define TASK_STOPPED		4	/* 停止状态 */

#ifndef NULL
#define NULL ((void *) 0)
#endif

/* 外部函数声明 */
extern int copy_page_tables(unsigned long from, unsigned long to, long size); /* 复制页表 */
extern int free_page_tables(unsigned long from, long size); /* 释放页表 */

extern void sched_init(void);		/* 调度器初始化 */
extern void schedule(void);		/* 进程调度 */
extern void trap_init(void);		/* 异常处理初始化 */
extern void panic(const char * str);	/* 内核恐慌函数 */
extern int tty_write(unsigned minor,char * buf,int count); /* 终端写函数 */

/* 函数指针类型定义 */
typedef int (*fn_ptr)();

/* 协处理器结构 */
struct i387_struct {
	long	cwd;			/* 控制字 */
	long	swd;			/* 状态字 */
	long	twd;			/* 标记字 */
	long	fip;			/* 指令指针偏移 */
	long	fcs;			/* 代码段选择符 */
	long	foo;			/* 操作数偏移 */
	long	fos;			/* 操作数段选择符 */
	long	st_space[20];		/* 8个浮点寄存器，每个10字节 = 80字节 */
};

/* 任务状态段结构 */
struct tss_struct {
	long	back_link;	/* 前一个链接，高16位为0 */
	long	esp0;		/* 0级堆栈指针 */
	long	ss0;		/* 0级堆栈段，高16位为0 */
	long	esp1;		/* 1级堆栈指针 */
	long	ss1;		/* 1级堆栈段，高16位为0 */
	long	esp2;		/* 2级堆栈指针 */
	long	ss2;		/* 2级堆栈段，高16位为0 */
	long	cr3;		/* 页目录基址寄存器 */
	long	eip;		/* 指令指针 */
	long	eflags;		/* 标志寄存器 */
	long	eax,ecx,edx,ebx; /* 通用寄存器 */
	long	esp;		/* 堆栈指针 */
	long	ebp;		/* 基址指针 */
	long	esi;		/* 源变址寄存器 */
	long	edi;		/* 目的变址寄存器 */
	long	es;		/* 附加段，高16位为0 */
	long	cs;		/* 代码段，高16位为0 */
	long	ss;		/* 堆栈段，高16位为0 */
	long	ds;		/* 数据段，高16位为0 */
	long	fs;		/* 附加段，高16位为0 */
	long	gs;		/* 附加段，高16位为0 */
	long	ldt;		/* 局部描述符表，高16位为0 */
	long	trace_bitmap;	/* 位图：跟踪位0，位图16-31 */
	struct i387_struct i387; /* 协处理器状态 */
};

/* 任务结构 */
struct task_struct {
/* 以下字段是硬编码的，不要修改 */
	long state;	/* -1不可运行，0可运行，>0已停止 */
	long counter;	/* 调度计数器 */
	long priority;	/* 优先级 */
	long signal;	/* 信号位图 */
	fn_ptr sig_restorer; /* 信号恢复函数 */
	fn_ptr sig_fn[32]; /* 信号处理函数数组 */
/* 各种字段 */
	int exit_code;	/* 退出码 */
	unsigned long end_code,end_data,brk,start_stack; /* 代码段、数据段、堆、栈的结束地址 */
	long pid,father,pgrp,session,leader; /* 进程ID、父进程ID、进程组、会话、组长标志 */
	unsigned short uid,euid,suid; /* 用户ID、有效用户ID、保存的用户ID */
	unsigned short gid,egid,sgid; /* 组ID、有效组ID、保存的组ID */
	long alarm;	/* 报警时间 */
	long utime,stime,cutime,cstime,start_time; /* 用户态时间、内核态时间、子进程用户态时间、子进程内核态时间、启动时间 */
	unsigned short used_math; /* 是否使用了协处理器 */
/* 文件系统信息 */
	int tty;		/* 终端号，-1表示无终端，必须是有符号数 */
	unsigned short umask; /* 文件权限掩码 */
	struct m_inode * pwd; /* 当前工作目录inode */
	struct m_inode * root; /* 根目录inode */
	unsigned long close_on_exec; /* 执行时关闭文件描述符位图 */
	struct file * filp[20]; /* 文件描述符表 */
/* 该任务的局部描述符表：0-空，1-cs，2-ds&ss */
	struct desc_struct ldt[3];
/* 该任务的任务状态段 */
	struct tss_struct tss;
};

/*
 * INIT_TASK用于设置第一个任务表，修改时需谨慎！
 * 基址=0，限制=0x9ffff（=640kB）
 */
#define INIT_TASK \
/* 状态等 */	{ 0,15,15, \
/* 信号 */	0,NULL,{(fn_ptr) 0,}, \
/* ec,brk... */	0,0,0,0,0, \
/* pid等 */	0,-1,0,0,0, \
/* uid等 */	0,0,0,0,0,0, \
/* 报警 */	0,0,0,0,0,0, \
/* 数学协处理器 */	0, \
/* 文件系统信息 */	-1,0133,NULL,NULL,0, \
/* 文件指针 */	{NULL,}, \
	{ \
		{0,0}, \
/* ldt */	{0x9f,0xc0fa00}, \
		{0x9f,0xc0f200}, \
	}, \
/* tss */	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\
	 0,0,0,0,0,0,0,0, \
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
	 _LDT(0),0x80000000, \
		{} \
	}, \
}

/* 全局变量声明 */
extern struct task_struct *task[NR_TASKS]; /* 任务数组 */
extern struct task_struct *last_task_used_math; /* 最后使用协处理器的任务 */
extern struct task_struct *current; /* 当前任务 */
extern long volatile jiffies; /* 系统启动以来的时钟节拍数 */
extern long startup_time; /* 系统启动时间 */

/* 获取当前时间 */
#define CURRENT_TIME (startup_time+jiffies/HZ)

/* 函数声明 */
extern void sleep_on(struct task_struct ** p); /* 睡眠 */
extern void interruptible_sleep_on(struct task_struct ** p); /* 可中断睡眠 */
extern void wake_up(struct task_struct ** p); /* 唤醒 */

/*
 * GDT中TSS的入口位置。0-空，1-cs，2-ds，3-系统调用
 * 4-TSS0，5-LDT0，6-TSS1等...
 */
#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))
#define str(n) \
__asm__("str %%ax\n\t" \
	"subl %2,%%eax\n\t" \
	"shrl $4,%%eax" \
	:"=a" (n) \
	:"a" (0),"i" (FIRST_TSS_ENTRY<<3))
/*
 * switch_to(n)应该切换到任务nr n，首先检查n是否是当前任务，
 * 如果是则不执行任何操作。如果切换到的任务最近使用了数学协处理器，
 * 则清除TS标志。
 */
#define switch_to(n) {\
struct {long a,b;} __tmp; \
__asm__("cmpl %%ecx,_current\n\t" \
	"je 1f\n\t" \
	"xchgl %%ecx,_current\n\t" \
	"movw %%dx,%1\n\t" \
	"ljmp %0\n\t" \
	"cmpl %%ecx,%2\n\t" \
	"jne 1f\n\t" \
	"clts\n" \
	"1:" \
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \
	"m" (last_task_used_math),"d" _TSS(n),"c" ((long) task[n])); \
}

/* 页面对齐 */
#define PAGE_ALIGN(n) (((n)+0xfff)&0xfffff000)

/* 设置段基址 */
#define _set_base(addr,base) \
__asm__("movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %%dl,%1\n\t" \
	"movb %%dh,%2" \
	::"m" (*((addr)+2)), \
	  "m" (*((addr)+4)), \
	  "m" (*((addr)+7)), \
	  "d" (base) \
	:"dx")

/* 设置段界限 */
#define _set_limit(addr,limit) \
__asm__("movw %%dx,%0\n\t" \
	"rorl $16,%%edx\n\t" \
	"movb %1,%%dh\n\t" \
	"andb $0xf0,%%dh\n\t" \
	"orb %%dh,%%dl\n\t" \
	"movb %%dl,%1" \
	::"m" (*(addr)), \
	  "m" (*((addr)+6)), \
	  "d" (limit) \
	:"dx")

#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , base )
#define set_limit(ldt,limit) _set_limit( ((char *)&(ldt)) , (limit-1)>>12 )

/* 获取段基址 */
#define _get_base(addr) ({\
unsigned long __base; \
__asm__("movb %3,%%dh\n\t" \
	"movb %2,%%dl\n\t" \
	"shll $16,%%edx\n\t" \
	"movw %1,%%dx" \
	:"=d" (__base) \
	:"m" (*((addr)+2)), \
	 "m" (*((addr)+4)), \
	 "m" (*((addr)+7))); \
__base;})

#define get_base(ldt) _get_base( ((char *)&(ldt)) )

/* 获取段界限 */
#define get_limit(segment) ({ \
unsigned long __limit; \
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
__limit;})

#endif