#include <signal.h>

#include <linux/config.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <asm/system.h>

// 进程退出函数声明
int do_exit(long code);

// 使TLB（Translation Lookaside Buffer）无效的宏定义
#define invalidate() \
__asm__("movl %%eax,%%cr3"::"a" (0))

// 根据BUFFER_END定义LOW_MEM的值
#if (BUFFER_END < 0x100000)
#define LOW_MEM 0x100000
#else
#define LOW_MEM BUFFER_END
#endif

/* 这些不应该被修改 - 它们是从上面计算得出的 */
#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEM)  // 分页内存大小
#define PAGING_PAGES (PAGING_MEMORY/4096)      // 分页页数
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)    // 地址到页号的映射

// 如果分页页数小于10，则报错
#if (PAGING_PAGES < 10)
#error "Won't work"
#endif

// 复制页面的宏定义
#define copy_page(from,to) \
__asm__("cld ; rep ; movsl"::"S" (from),"D" (to),"c" (1024):"cx","di","si")

// 内存映射数组，用于跟踪页面使用情况
static unsigned short mem_map [ PAGING_PAGES ] = {0,};

/*
 * 获取第一个（实际上是最后一个:-)空闲页面的物理地址，并标记为已使用。
 * 如果没有空闲页面，则返回0。
 */
unsigned long get_free_page(void)
{
register unsigned long __res asm("ax");

__asm__("std ; repne ; scasw\n\t"
	"jne 1f\n\t"
	"movw $1,2(%%edi)\n\t"
	"sall $12,%%ecx\n\t"
	"movl %%ecx,%%edx\n\t"
	"addl %2,%%edx\n\t"
	"movl $1024,%%ecx\n\t"
	"leal 4092(%%edx),%%edi\n\t"
	"rep ; stosl\n\t"
	"movl %%edx,%%eax\n"
	"1:"
	:"=a" (__res)
	:"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES),
	"D" (mem_map+PAGING_PAGES-1)
	:"di","cx","dx");
return __res;
}

/*
 * 释放物理地址为'addr'的内存页面。由'free_page_tables()'使用
 */
void free_page(unsigned long addr)
{
	if (addr<LOW_MEM) return;
	if (addr>HIGH_MEMORY)
		panic("trying to free nonexistent page");
	addr -= LOW_MEM;
	addr >>= 12;
	if (mem_map[addr]--) return;
	mem_map[addr]=0;
	panic("trying to free free page");
}

/*
 * 这个函数释放连续的页表块，由'exit()'所需。
 * 与copy_page_tables()一样，这只处理4Mb的块。
 */
int free_page_tables(unsigned long from,unsigned long size)
{
	unsigned long *pg_table;
	unsigned long * dir, nr;

	if (from & 0x3fffff)
		panic("free_page_tables called with wrong alignment");
	if (!from)
		panic("Trying to free up swapper memory space");
	size = (size + 0x3fffff) >> 22;
	dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */
	for ( ; size-->0 ; dir++) {
		if (!(1 & *dir))
			continue;
		pg_table = (unsigned long *) (0xfffff000 & *dir);
		for (nr=0 ; nr<1024 ; nr++) {
			if (1 & *pg_table)
				free_page(0xfffff000 & *pg_table);
			*pg_table = 0;
			pg_table++;
		}
		free_page(0xfffff000 & *dir);
		*dir = 0;
	}
	invalidate();
	return 0;
}

/*
 * 好了，这是mm中最复杂的函数之一。它通过只复制页面来复制线性地址范围。
 * 希望这是无bug的，因为这个函数我不想调试 :-)
 *
 * 注意！我们不复制任何内存块 - 地址必须能被4Mb整除（一个页目录项），
 * 因为这使函数更容易。它只被fork使用。
 *
 * 注意2！！当from==0时，我们正在为第一次fork()复制内核空间。
 * 那时我们不想复制整个页目录项，因为那会导致严重的内存浪费 -
 * 我们只复制前160页 - 640kB。即使这比我们需要的多，但它不会占用更多内存 -
 * 我们在低1Mb范围内不使用写时复制，所以页面可以与内核共享。
 * 因此nr=xxxx的特殊情况。
 */
int copy_page_tables(unsigned long from,unsigned long to,long size)
{
	unsigned long * from_page_table;
	unsigned long * to_page_table;
	unsigned long this_page;
	unsigned long * from_dir, * to_dir;
	unsigned long nr;

	if ((from&0x3fffff) || (to&0x3fffff))
		panic("copy_page_tables called with wrong alignment");
	from_dir = (unsigned long *) ((from>>20) & 0xffc); /* _pg_dir = 0 */
	to_dir = (unsigned long *) ((to>>20) & 0xffc);
	size = ((unsigned) (size+0x3fffff)) >> 22;
	for( ; size-->0 ; from_dir++,to_dir++) {
		if (1 & *to_dir)
			panic("copy_page_tables: already exist");
		if (!(1 & *from_dir))
			continue;
		from_page_table = (unsigned long *) (0xfffff000 & *from_dir);
		if (!(to_page_table = (unsigned long *) get_free_page()))
			return -1;	/* 内存不足，参见释放 */
		*to_dir = ((unsigned long) to_page_table) | 7;
		nr = (from==0)?0xA0:1024;
		for ( ; nr-- > 0 ; from_page_table++,to_page_table++) {
			this_page = *from_page_table;
			if (!(1 & this_page))
				continue;
			this_page &= ~2;
			*to_page_table = this_page;
			if (this_page > LOW_MEM) {
				*from_page_table = this_page;
				this_page -= LOW_MEM;
				this_page >>= 12;
				mem_map[this_page]++;
			}
		}
	}
	invalidate();
	return 0;
}

/*
 * 这个函数将一个页面放入内存中的指定地址。
 * 它返回获取的页面的物理地址，如果内存不足则返回0
 * （在尝试访问页表或页面时）。
 */
unsigned long put_page(unsigned long page,unsigned long address)
{
	unsigned long tmp, *page_table;

/* 注意！！！这利用了_pg_dir=0的事实 */

	if (page < LOW_MEM || page > HIGH_MEMORY)
		printk("Trying to put page %p at %p\n",page,address);
	if (mem_map[(page-LOW_MEM)>>12] != 1)
		printk("mem_map disagrees with %p at %p\n",page,address);
	page_table = (unsigned long *) ((address>>20) & 0xffc);
	if ((*page_table)&1)
		page_table = (unsigned long *) (0xfffff000 & *page_table);
	else {
		if (!(tmp=get_free_page()))
			return 0;
		*page_table = tmp|7;
		page_table = (unsigned long *) tmp;
	}
	page_table[(address>>12) & 0x3ff] = page | 7;
	return page;
}

// 解除写保护页面
void un_wp_page(unsigned long * table_entry)
{
	unsigned long old_page,new_page;

	old_page = 0xfffff000 & *table_entry;
	if (old_page >= LOW_MEM && mem_map[MAP_NR(old_page)]==1) {
		*table_entry |= 2;
		return;
	}
	if (!(new_page=get_free_page()))
		do_exit(SIGSEGV);
	if (old_page >= LOW_MEM)
		mem_map[MAP_NR(old_page)]--;
	*table_entry = new_page | 7;
	copy_page(old_page,new_page);
}	

/*
 * 这个例程处理当前页面，当用户尝试写入共享页面时。
 * 通过将页面复制到新地址并减少旧页面的共享页面计数器来完成。
 */
void do_wp_page(unsigned long error_code,unsigned long address)
{
	un_wp_page((unsigned long *)
		(((address>>10) & 0xffc) + (0xfffff000 &
		*((unsigned long *) ((address>>20) &0xffc)))));

}

// 写验证函数
void write_verify(unsigned long address)
{
	unsigned long page;

	if (!( (page = *((unsigned long *) ((address>>20) & 0xffc)) )&1))
		return;
	page &= 0xfffff000;
	page += ((address>>10) & 0xffc);
	if ((3 & *(unsigned long *) page) == 1)  /* 非可写，当前 */
		un_wp_page((unsigned long *) page);
	return;
}

// 处理缺页异常
void do_no_page(unsigned long error_code,unsigned long address)
{
	unsigned long tmp;

	if (tmp=get_free_page())
		if (put_page(tmp,address))
			return;
	do_exit(SIGSEGV);
}

// 计算内存使用情况
void calc_mem(void)
{
	int i,j,k,free=0;
	long * pg_tbl;

	for(i=0 ; i<PAGING_PAGES ; i++)
		if (!mem_map[i]) free++;
	printk("%d pages free (of %d)\n\r",free,PAGING_PAGES);
	for(i=2 ; i<1024 ; i++) {
		if (1&pg_dir[i]) {
			pg_tbl=(long *) (0xfffff000 & pg_dir[i]);
			for(j=k=0 ; j<1024 ; j++)
				if (pg_tbl[j]&1)
					k++;
			printk("Pg-dir[%d] uses %d pages\n",i,k);
		}
	}
}