/*
 *  'buffer.c'实现了缓冲区缓存功能。通过从不让中断改变缓冲区
 * （当然除了数据之外）来避免竞争条件，而是让调用者来做。
 * 注意！由于中断可以唤醒调用者，需要一些cli-sti序列来检查
 * sleep-on调用。这些应该非常快（我希望）。
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>

// 检查BUFFER_END值是否正确对齐
#if (BUFFER_END & 0xfff)
#error "Bad BUFFER_END value"
#endif

// 检查BUFFER_END值是否在保留内存区域
#if (BUFFER_END > 0xA0000 && BUFFER_END <= 0x100000)
#error "Bad BUFFER_END value"
#endif

// 外部变量声明
extern int end;
// 缓冲区起始地址
struct buffer_head * start_buffer = (struct buffer_head *) &end;
// 哈希表，用于快速查找缓冲区
struct buffer_head * hash_table[NR_HASH];
// 空闲缓冲区链表
static struct buffer_head * free_list;
// 等待缓冲区的进程
static struct task_struct * buffer_wait = NULL;
// 缓冲区数量
int NR_BUFFERS = 0;

// 等待缓冲区解锁的内联函数
static inline void wait_on_buffer(struct buffer_head * bh)
{
	cli();
	while (bh->b_lock)
		sleep_on(&bh->b_wait);
	sti();
}

// 系统同步调用，将所有脏缓冲区写入磁盘
int sys_sync(void)
{
	int i;
	struct buffer_head * bh;

	sync_inodes();		/* 将inode写入缓冲区 */
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

// 同步指定设备的所有缓冲区
static int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;

	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

// 哈希函数，计算设备和块号的哈希值
#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
// 获取哈希表中的缓冲区
#define hash(dev,block) hash_table[_hashfn(dev,block)]

// 从队列中移除缓冲区的内联函数
static inline void remove_from_queues(struct buffer_head * bh)
{
/* 从哈希队列中移除 */
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
/* 从空闲列表中移除 */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)
		free_list = bh->b_next_free;
}

// 将缓冲区插入队列的内联函数
static inline void insert_into_queues(struct buffer_head * bh)
{
/* 放在空闲列表的末尾 */
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* 如果缓冲区有设备，则将其放入新的哈希队列 */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;
}

// 查找缓冲区
static struct buffer_head * find_buffer(int dev, int block)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
			return tmp;
	return NULL;
}

/*
 * 为什么这样实现，我听到你说...原因是竞争条件。
 * 由于我们不锁定缓冲区（除非我们在读取它们），
 * 在我们睡眠时可能会发生一些事情（例如读取错误
 * 会强制它变坏）。这目前不应该真正发生，但
 * 代码已经准备好了。
 */
// 获取哈希表中的缓冲区
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;

repeat:
	if (!(bh=find_buffer(dev,block)))
		return NULL;
	bh->b_count++;
	wait_on_buffer(bh);
	if (bh->b_dev != dev || bh->b_blocknr != block) {
		brelse(bh);
		goto repeat;
	}
	return bh;
}

/*
 * 好了，这是getblk，它不是很清楚，再次是为了阻碍
 * 竞争条件。大部分代码很少使用，（即重复），
 * 所以它应该比看起来更高效。
 */
// 获取缓冲区块
struct buffer_head * getblk(int dev,int block)
{
	struct buffer_head * tmp;

repeat:
	if (tmp=get_hash_table(dev,block))
		return tmp;
	tmp = free_list;
	do {
		if (!tmp->b_count) {
			wait_on_buffer(tmp);	/* 我们仍然必须等待 */
			if (!tmp->b_count)	/* 等待它，它可能是脏的 */
				break;
		}
		tmp = tmp->b_next_free;
	} while (tmp != free_list || (tmp=NULL));
	/* 孩子们，不要在家里尝试这个^^^^^。魔法 */
	if (!tmp) {
		printk("Sleeping on free buffer ..");
		sleep_on(&buffer_wait);
		printk("ok\n");
		goto repeat;
	}
	tmp->b_count++;
	remove_from_queues(tmp);
/*
 * 现在，当我们知道没有人能访问这个节点时（因为它已从
 * 空闲列表中移除），我们将其写入磁盘。我们可以在这里睡眠
 * 而不用担心竞争条件。
 */
	if (tmp->b_dirt)
		sync_dev(tmp->b_dev);
/* 更新缓冲区内容 */
	tmp->b_dev=dev;
	tmp->b_blocknr=block;
	tmp->b_dirt=0;
	tmp->b_uptodate=0;
/* 注意！！当我们可能在sync_dev()中睡眠时，其他人可能已经
 * 添加了"这个"块，所以要检查一下。感谢上帝有goto语句。
 */
	if (find_buffer(dev,block)) {
		tmp->b_dev=0;		/* 好的，其他人已经抢先了 */
		tmp->b_blocknr=0;	/* 到了它 - 释放这个块并 */
		tmp->b_count=0;		/* 重试 */
		insert_into_queues(tmp);
		goto repeat;
	}
/* 然后插入到正确的位置 */
	insert_into_queues(tmp);
	return tmp;
}

// 释放缓冲区
void brelse(struct buffer_head * buf)
{
	if (!buf)
		return;
	wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	wake_up(&buffer_wait);
}

/*
 * bread()读取指定的块并返回包含它的缓冲区。
 * 如果块不可读，则返回NULL。
 */
// 读取块
struct buffer_head * bread(int dev,int block)
{
	struct buffer_head * bh;

	if (!(bh=getblk(dev,block)))
		panic("bread: getblk returned NULL\n");
	if (bh->b_uptodate)
		return bh;
	ll_rw_block(READ,bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return (NULL);
}

// 缓冲区初始化函数
void buffer_init(void)
{
	struct buffer_head * h = start_buffer;
	void * b = (void *) BUFFER_END;
	int i;

	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {
		h->b_dev = 0;
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;
		h->b_prev_free = h-1;
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;
		if (b == (void *) 0x100000)
			b = (void *) 0xA0000;
	}
	h--;
	free_list = start_buffer;
	free_list->b_prev_free = h;
	h->b_next_free = free_list;
	for (i=0;i<NR_HASH;i++)
		hash_table[i]=NULL;
}	