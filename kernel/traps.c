/*
 * 'Traps.c'处理硬件陷阱和故障，在我们保存了一些状态到'asm.s'之后。
 * 目前主要是一个调试辅助工具，将扩展为主要杀死违规进程
 * (可能通过给它发送信号，但必要时可能直接杀死它)。
 */
#include <string.h>

#include <linux/head.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/segment.h>

// 从指定段获取字节的宏定义
#define get_seg_byte(seg,addr) ({ \
register char __res; \
__asm__("push %%fs;mov %%ax,%%fs;movb %%fs:%2,%%al;pop %%fs" \
	:"=a" (__res):"0" (seg),"m" (*(addr))); \
__res;})

// 从指定段获取长整型的宏定义
#define get_seg_long(seg,addr) ({ \
register unsigned long __res; \
__asm__("push %%fs;mov %%ax,%%fs;movl %%fs:%2,%%eax;pop %%fs" \
	:"=a" (__res):"0" (seg),"m" (*(addr))); \
__res;})

// 获取fs段寄存器值的宏定义
#define _fs() ({ \
register unsigned short __res; \
__asm__("mov %%fs,%%ax":"=a" (__res):); \
__res;})

// 进程退出函数声明
int do_exit(long code);

// 页面异常处理函数声明
void page_exception(void);

// 各种异常处理函数声明
void divide_error(void);                  // 除法错误
void debug(void);                         // 调试异常
void nmi(void);                           // 非屏蔽中断
void int3(void);                          // 断点中断
void overflow(void);                      // 溢出
void bounds(void);                        // 边界检查
void invalid_op(void);                    // 无效操作码
void device_not_available(void);          // 设备不可用
void double_fault(void);                  // 双重故障
void coprocessor_segment_overrun(void);   // 协处理器段越界
void invalid_TSS(void);                   // 无效TSS
void segment_not_present(void);           // 段不存在
void stack_segment(void);                 // 栈段异常
void general_protection(void);            // 一般保护异常
void page_fault(void);                    // 页面故障
void coprocessor_error(void);             // 协处理器错误
void reserved(void);                      // 保留异常

// 内核崩溃处理函数
static void die(char * str,long esp_ptr,long nr)
{
	long * esp = (long *) esp_ptr;
	int i;

	printk("%s: %04x\n\r",str,nr&0xffff);
	printk("EIP:\t%04x:%p\nEFLAGS:\t%p\nESP:\t%04x:%p\n",
		esp[1],esp[0],esp[2],esp[4],esp[3]);
	printk("fs: %04x\n",_fs());
	printk("base: %p, limit: %p\n",get_base(current->ldt[1]),get_limit(0x17));
	if (esp[4] == 0x17) {
		printk("Stack: ");
		for (i=0;i<4;i++)
			printk("%p ",get_seg_long(0x17,i+(long *)esp[3]));
		printk("\n");
	}
	str(i);
	printk("Pid: %d, process nr: %d\n\r",current->pid,0xffff & i);
	for(i=0;i<10;i++)
		printk("%02x ",0xff & get_seg_byte(esp[1],(i+(char *)esp[0])));
	printk("\n\r");
	do_exit(11);		/* 段异常 */
}

// 双重故障处理
void do_double_fault(long esp, long error_code)
{
	die("double fault",esp,error_code);
}

// 一般保护异常处理
void do_general_protection(long esp, long error_code)
{
	die("general protection",esp,error_code);
}

// 除法错误处理
void do_divide_error(long esp, long error_code)
{
	die("divide error",esp,error_code);
}

// 断点中断处理
void do_int3(long * esp, long error_code,
		long fs,long es,long ds,
		long ebp,long esi,long edi,
		long edx,long ecx,long ebx,long eax)
{
	int tr;

	__asm__("str %%ax":"=a" (tr):"0" (0));
	printk("eax\t\tebx\t\tecx\t\tedx\n\r%8x\t%8x\t%8x\t%8x\n\r",
		eax,ebx,ecx,edx);
	printk("esi\t\tedi\t\tebp\t\tesp\n\r%8x\t%8x\t%8x\t%8x\n\r",
		esi,edi,ebp,(long) esp);
	printk("\n\rds\tes\tfs\ttr\n\r%4x\t%4x\t%4x\t%4x\n\r",
		ds,es,fs,tr);
	printk("EIP: %8x   CS: %4x  EFLAGS: %8x\n\r",esp[0],esp[1],esp[2]);
}

// 非屏蔽中断处理
void do_nmi(long esp, long error_code)
{
	die("nmi",esp,error_code);
}

// 调试异常处理
void do_debug(long esp, long error_code)
{
	die("debug",esp,error_code);
}

// 溢出处理
void do_overflow(long esp, long error_code)
{
	die("overflow",esp,error_code);
}

// 边界检查处理
void do_bounds(long esp, long error_code)
{
	die("bounds",esp,error_code);
}

// 无效操作数处理
void do_invalid_op(long esp, long error_code)
{
	die("invalid operand",esp,error_code);
}

// 设备不可用处理
void do_device_not_available(long esp, long error_code)
{
	die("device not available",esp,error_code);
}

// 协处理器段越界处理
void do_coprocessor_segment_overrun(long esp, long error_code)
{
	die("coprocessor segment overrun",esp,error_code);
}

// 无效TSS处理
void do_invalid_TSS(long esp,long error_code)
{
	die("invalid TSS",esp,error_code);
}

// 段不存在处理
void do_segment_not_present(long esp,long error_code)
{
	die("segment not present",esp,error_code);
}

// 栈段异常处理
void do_stack_segment(long esp,long error_code)
{
	die("stack segment",esp,error_code);
}

// 协处理器错误处理
void do_coprocessor_error(long esp, long error_code)
{
	die("coprocessor error",esp,error_code);
}

// 保留异常处理
void do_reserved(long esp, long error_code)
{
	die("reserved (15,17-31) error",esp,error_code);
}

// 异常处理初始化函数
void trap_init(void)
{
	int i;

	set_trap_gate(0,&divide_error);
	set_trap_gate(1,&debug);
	set_trap_gate(2,&nmi);
	set_system_gate(3,&int3);	/* int3-5可以从所有地方调用 */
	set_system_gate(4,&overflow);
	set_system_gate(5,&bounds);
	set_trap_gate(6,&invalid_op);
	set_trap_gate(7,&device_not_available);
	set_trap_gate(8,&double_fault);
	set_trap_gate(9,&coprocessor_segment_overrun);
	set_trap_gate(10,&invalid_TSS);
	set_trap_gate(11,&segment_not_present);
	set_trap_gate(12,&stack_segment);
	set_trap_gate(13,&general_protection);
	set_trap_gate(14,&page_fault);
	set_trap_gate(15,&reserved);
	set_trap_gate(16,&coprocessor_error);
	for (i=17;i<32;i++)
		set_trap_gate(i,&reserved);
/*	__asm__("movl $0x3ff000,%%eax\n\t"
		"movl %%eax,%%db0\n\t"
		"movl $0x000d0303,%%eax\n\t"
		"movl %%eax,%%db7"
		:::"ax");*/
}