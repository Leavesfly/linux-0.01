# IDT初始化与门描述符设置

<cite>
**本文档引用的文件**  
- [traps.c](file://kernel/traps.c)
- [system.h](file://include/asm/system.h)
- [head.h](file://include/linux/head.h)
</cite>

## 目录
1. [引言](#引言)
2. [IDT初始化流程](#idt初始化流程)
3. [门描述符设置机制](#门描述符设置机制)
4. [_set_gate宏的汇编实现](#_set_gate宏的汇编实现)
5. [IDT表项的二进制格式](#idt表项的二进制格式)
6. [系统调用门与陷阱门的区别](#系统调用门与陷阱门的区别)
7. [调试建议](#调试建议)

## 引言
中断描述符表（IDT）是x86架构中用于管理中断和异常的核心数据结构。在Linux 0.01内核中，`trap_init()`函数负责初始化IDT，为各个异常向量设置对应的处理程序。本文将深入分析该过程，重点解析`set_trap_gate()`、`set_system_gate()`等宏的实现机制，以及IDT表项的构造方式。

## IDT初始化流程

`trap_init()`函数在内核启动过程中被调用，其主要任务是为前32个异常向量（0-31）设置对应的异常处理程序。这些异常向量对应于处理器定义的各类硬件异常，如除零错误、页错误等。

函数通过一系列宏调用为每个异常向量安装处理函数。例如，向量0（除法错误）由`divide_error`处理，向量14（页错误）由`page_fault`处理。对于保留的异常向量（15和17-31），统一指向`reserved`处理函数。

```c
void trap_init(void)
{
	int i;

	set_trap_gate(0,&divide_error);
	set_trap_gate(1,&debug);
	set_trap_gate(2,&nmi);
	set_system_gate(3,&int3);
	set_system_gate(4,&overflow);
	set_system_gate(5,&bounds);
	// ... 其他异常设置
	for (i=17;i<32;i++)
		set_trap_gate(i,&reserved);
}
```

**Section sources**
- [traps.c](file://kernel/traps.c#L164-L197)

## 门描述符设置机制

Linux 0.01使用三个宏来设置不同类型的门描述符：
- `set_trap_gate(n, addr)`：设置陷阱门（DPL=0）
- `set_system_gate(n, addr)`：设置系统门（DPL=3）
- `set_intr_gate(n, addr)`：设置中断门（DPL=0）

这些宏最终都调用底层的`_set_gate()`宏来完成实际的描述符写入操作。它们的区别在于传递给`_set_gate()`的参数不同，特别是DPL（描述符特权级）和类型字段。

例如，`set_system_gate()`允许用户态程序通过`int 0x80`发起系统调用，因此其DPL设为3（用户态），而普通异常处理程序的DPL为0（内核态）。

**Section sources**
- [system.h](file://include/asm/system.h#L50-L60)

## _set_gate宏的汇编实现

`_set_gate`宏是IDT初始化的核心，它直接使用内联汇编构造并写入门描述符。该宏接受四个参数：门地址、类型、DPL和处理程序地址。

```c
#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))
```

该汇编代码执行以下操作：
1. 将段选择子（0x08，内核代码段）和偏移地址的高16位放入EAX
2. 构造低32位描述符：包含偏移地址低16位、段选择子、保留位、DPL、P位和类型码
3. 将高32位和低32位分别写入门描述符的两个双字位置

这种实现方式高效且直接，避免了函数调用开销。

**Section sources**
- [system.h](file://include/asm/system.h#L40-L50)

## IDT表项的二进制格式

在保护模式下，每个IDT表项（门描述符）为8字节，其二进制格式如下：

| 字段 | 位范围 | 说明 |
|------|--------|------|
| 偏移地址低16位 | 0-15 | 中断处理程序的偏移地址 |
| 段选择子 | 16-31 | 代码段选择子（通常为0x08） |
| 保留 | 32-39 | 保留位，设为0 |
| 类型 | 40-43 | 门类型（14=中断门，15=陷阱门） |
| DPL | 44-45 | 描述符特权级（0-3） |
| P | 46 | 存在位（1=有效） |
| 偏移地址高16位 | 47-63 | 中断处理程序偏移地址高16位 |

该格式符合Intel x86架构规范，确保处理器能正确解析并跳转到相应的处理程序。

**Section sources**
- [system.h](file://include/asm/system.h#L40-L50)
- [head.h](file://include/linux/head.h#L5-L10)

## 系统调用门与陷阱门的区别

系统调用门（通过`set_system_gate`设置）与普通陷阱门的关键区别在于DPL：

- **陷阱门（DPL=0）**：只能由内核态代码触发，用于处理异常。用户程序无法直接调用。
- **系统调用门（DPL=3）**：允许用户态程序通过`int`指令调用，用于实现系统调用机制。

在`sched.c`中，`sched_init()`函数调用`set_system_gate(0x80,&system_call)`，为`int 0x80`指令设置系统调用入口，使得用户程序可以通过该中断向量进入内核态执行系统调用。

```mermaid
flowchart LR
UserApp["用户程序\n(特权级3)"] --> |int 0x80| SystemGate["系统调用门\nDPL=3"]
SystemGate --> Kernel["system_call\n内核处理程序\n(特权级0)"]
style SystemGate fill:#f9f,stroke:#333
```

**Diagram sources**
- [system.h](file://include/asm/system.h#L55-L60)
- [sched.c](file://kernel/sched.c#L250-L253)

**Section sources**
- [system.h](file://include/asm/system.h#L55-L60)
- [sched.c](file://kernel/sched.c#L250-L253)

## 调试建议

验证IDT表项是否正确设置的方法包括：

1. **静态检查**：通过反汇编或调试器查看IDT内存区域的内容，确认各表项的段选择子、偏移地址、DPL和类型字段是否正确。
2. **动态测试**：触发特定异常（如除零错误）观察是否跳转到预期的处理程序。
3. **打印诊断**：在`die()`函数中添加额外的寄存器和状态打印，帮助分析异常上下文。
4. **特权级测试**：尝试从用户态调用不同DPL的门描述符，验证特权级检查机制是否正常工作。

这些方法可以帮助开发者确认IDT初始化的正确性，并快速定位相关问题。

**Section sources**
- [traps.c](file://kernel/traps.c#L60-L100)
- [system.h](file://include/asm/system.h#L40-L60)