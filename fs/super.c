/*
 * super.c包含处理超级块表的代码。
 */
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>

/* set_bit使用setb，因为gas不识别setc */
// 设置位的宏定义
#define set_bit(bitnr,addr) ({ \
register int __res __asm__("ax"); \
__asm__("bt %2,%3;setb %%al":"=a" (__res):"a" (0),"r" (bitnr),"m" (*(addr))); \
__res; })

// 超级块表
struct super_block super_block[NR_SUPER];

// 挂载设备的函数
struct super_block * do_mount(int dev)
{
	struct super_block * p;
	struct buffer_head * bh;
	int i,block;

	// 查找空闲的超级块
	for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++ )
		if (!(p->s_dev))
			break;
	p->s_dev = -1;		/* 标记为正在使用 */
	if (p >= &super_block[NR_SUPER])
		return NULL;
	// 读取设备的超级块（第1块）
	if (!(bh = bread(dev,1)))
		return NULL;
	*p = *((struct super_block *) bh->b_data);
	brelse(bh);
	// 检查魔数是否正确
	if (p->s_magic != SUPER_MAGIC) {
		p->s_dev = 0;
		return NULL;
	}
	// 初始化inode位图和数据块位图指针
	for (i=0;i<I_MAP_SLOTS;i++)
		p->s_imap[i] = NULL;
	for (i=0;i<Z_MAP_SLOTS;i++)
		p->s_zmap[i] = NULL;
	block=2;
	// 读取inode位图块
	for (i=0 ; i < p->s_imap_blocks ; i++)
		if (p->s_imap[i]=bread(dev,block))
			block++;
		else
			break;
	// 读取数据块位图块
	for (i=0 ; i < p->s_zmap_blocks ; i++)
		if (p->s_zmap[i]=bread(dev,block))
			block++;
		else
			break;
	// 检查是否所有位图块都读取成功
	if (block != 2+p->s_imap_blocks+p->s_zmap_blocks) {
		for(i=0;i<I_MAP_SLOTS;i++)
			brelse(p->s_imap[i]);
		for(i=0;i<Z_MAP_SLOTS;i++)
			brelse(p->s_zmap[i]);
		p->s_dev=0;
		return NULL;
	}
	// 标记根inode和根数据块为已使用
	p->s_imap[0]->b_data[0] |= 1;
	p->s_zmap[0]->b_data[0] |= 1;
	p->s_dev = dev;
	p->s_isup = NULL;
	p->s_imount = NULL;
	p->s_time = 0;
	p->s_rd_only = 0;
	p->s_dirt = 0;
	return p;
}

// 挂载根文件系统的函数
void mount_root(void)
{
	int i,free;
	struct super_block * p;
	struct m_inode * mi;

	// 检查inode结构大小是否正确
	if (32 != sizeof (struct d_inode))
		panic("bad i-node size");
	// 初始化文件表
	for(i=0;i<NR_FILE;i++)
		file_table[i].f_count=0;
	// 初始化超级块表
	for(p = &super_block[0] ; p < &super_block[NR_SUPER] ; p++)
		p->s_dev = 0;
	// 挂载根设备
	if (!(p=do_mount(ROOT_DEV)))
		panic("Unable to mount root");
	// 读取根inode
	if (!(mi=iget(ROOT_DEV,1)))
		panic("Unable to read root i-node");
	mi->i_count += 3 ;	/* 注意！逻辑上被使用4次，不是1次 */
	p->s_isup = p->s_imount = mi;
	current->pwd = mi;
	current->root = mi;
	// 统计空闲块数量
	free=0;
	i=p->s_nzones;
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_zmap[i>>13]->b_data))
			free++;
	printk("%d/%d free blocks\n\r",free,p->s_nzones);
	// 统计空闲inode数量
	free=0;
	i=p->s_ninodes+1;
	while (-- i >= 0)
		if (!set_bit(i&8191,p->s_imap[i>>13]->b_data))
			free++;
	printk("%d/%d free inodes\n\r",free,p->s_ninodes);
}