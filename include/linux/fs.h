/*
 * 这个文件定义了一些重要的文件表结构等
 */

#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

/* 设备定义如下：（与minix相同，因此我们可以使用minix文件系统）
 * 0 - 未使用（无设备）
 * 1 - /dev/mem
 * 2 - /dev/fd
 * 3 - /dev/hd
 * 4 - /dev/ttyx
 * 5 - /dev/tty
 * 6 - /dev/lp
 * 7 - 无名管道
 */

/* 判断是否为块设备 */
#define IS_BLOCKDEV(x) ((x)==2 || (x)==3)

/* 读写操作标志 */
#define READ 0
#define WRITE 1

/* 初始化缓冲区 */
void buffer_init(void);

/* 获取主设备号 */
#define MAJOR(a) (((unsigned)(a))>>8)
/* 获取次设备号 */
#define MINOR(a) ((a)&0xff)

/* 文件名最大长度 */
#define NAME_LEN 14

/* inode位图槽数量 */
#define I_MAP_SLOTS 8
/* 数据块位图槽数量 */
#define Z_MAP_SLOTS 8
/* 超级块魔数 */
#define SUPER_MAGIC 0x137F

/* 最大打开文件数 */
#define NR_OPEN 20
/* inode表大小 */
#define NR_INODE 32
/* 文件表大小 */
#define NR_FILE 64
/* 超级块表大小 */
#define NR_SUPER 8
/* 哈希表大小 */
#define NR_HASH 307
/* 缓冲区数量 */
#define NR_BUFFERS nr_buffers
/* 块大小 */
#define BLOCK_SIZE 1024
#ifndef NULL
#define NULL ((void *) 0)
#endif

/* 每个块中inode数量 */
#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))
/* 每个块中目录项数量 */
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

/* 缓冲区块类型定义 */
typedef char buffer_block[BLOCK_SIZE];

/* 缓冲区头部结构 */
struct buffer_head {
	char * b_data;			/* 指向数据块的指针（1024字节） */
	unsigned short b_dev;		/* 设备号（0表示空闲） */
	unsigned short b_blocknr;	/* 块号 */
	unsigned char b_uptodate;	/* 更新标志 */
	unsigned char b_dirt;		/* 脏标志：0-干净，1-脏 */
	unsigned char b_count;		/* 使用该块的用户数 */
	unsigned char b_lock;		/* 锁标志：0-正常，1-锁定 */
	struct task_struct * b_wait;	/* 等待该缓冲区的进程 */
	struct buffer_head * b_prev;	/* 链表前向指针 */
	struct buffer_head * b_next;	/* 链表后向指针 */
	struct buffer_head * b_prev_free; /* 空闲链表前向指针 */
	struct buffer_head * b_next_free; /* 空闲链表后向指针 */
};

/* 磁盘上的inode结构 */
struct d_inode {
	unsigned short i_mode;		/* 文件类型和访问权限 */
	unsigned short i_uid;		/* 用户ID */
	unsigned long i_size;		/* 文件大小（字节） */
	unsigned long i_time;		/* 修改时间 */
	unsigned char i_gid;		/* 组ID */
	unsigned char i_nlinks;		/* 链接数 */
	unsigned short i_zone[9];	/* 直接和间接数据块指针 */
};

/* 内存中的inode结构 */
struct m_inode {
	unsigned short i_mode;		/* 文件类型和访问权限 */
	unsigned short i_uid;		/* 用户ID */
	unsigned long i_size;		/* 文件大小（字节） */
	unsigned long i_mtime;		/* 修改时间 */
	unsigned char i_gid;		/* 组ID */
	unsigned char i_nlinks;		/* 链接数 */
	unsigned short i_zone[9];	/* 直接和间接数据块指针 */
/* 以下字段仅存在于内存中 */
	struct task_struct * i_wait;	/* 等待该inode的进程 */
	unsigned long i_atime;		/* 最后访问时间 */
	unsigned long i_ctime;		/* 创建时间 */
	unsigned short i_dev;		/* 设备号 */
	unsigned short i_num;		/* inode号 */
	unsigned short i_count;		/* 引用计数 */
	unsigned char i_lock;		/* 锁标志 */
	unsigned char i_dirt;		/* 脏标志 */
	unsigned char i_pipe;		/* 管道标志 */
	unsigned char i_mount;		/* 安装标志 */
	unsigned char i_seek;		/* 寻址标志 */
	unsigned char i_update;		/* 更新标志 */
};

/* 管道头部宏定义 */
#define PIPE_HEAD(inode) (((long *)((inode).i_zone))[0])
/* 管道尾部宏定义 */
#define PIPE_TAIL(inode) (((long *)((inode).i_zone))[1])
/* 管道大小宏定义 */
#define PIPE_SIZE(inode) ((PIPE_HEAD(inode)-PIPE_TAIL(inode))&(PAGE_SIZE-1))
/* 管道是否为空 */
#define PIPE_EMPTY(inode) (PIPE_HEAD(inode)==PIPE_TAIL(inode))
/* 管道是否已满 */
#define PIPE_FULL(inode) (PIPE_SIZE(inode)==(PAGE_SIZE-1))
/* 增加管道头部指针 */
#define INC_PIPE(head) \
__asm__("incl %0\n\tandl $4095,%0"::"m" (head))

/* 文件结构 */
struct file {
	unsigned short f_mode;		/* 文件模式 */
	unsigned short f_flags;		/* 文件标志 */
	unsigned short f_count;		/* 引用计数 */
	struct m_inode * f_inode;	/* 指向inode的指针 */
	off_t f_pos;			/* 文件位置 */
};

/* 超级块结构 */
struct super_block {
	unsigned short s_ninodes;	/* inode数量 */
	unsigned short s_nzones;	/* 数据块数量 */
	unsigned short s_imap_blocks;	/* inode位图块数 */
	unsigned short s_zmap_blocks;	/* 数据块位图块数 */
	unsigned short s_firstdatazone;	/* 第一个数据块号 */
	unsigned short s_log_zone_size;	/* log2(数据块大小) */
	unsigned long s_max_size;	/* 最大文件大小 */
	unsigned short s_magic;		/* 魔数 */
/* 以下字段仅存在于内存中 */
	struct buffer_head * s_imap[8];	/* inode位图缓冲区指针 */
	struct buffer_head * s_zmap[8];	/* 数据块位图缓冲区指针 */
	unsigned short s_dev;		/* 设备号 */
	struct m_inode * s_isup;	/* 被安装的inode */
	struct m_inode * s_imount;	/* 安装点的inode */
	unsigned long s_time;		/* 修改时间 */
	unsigned char s_rd_only;	/* 只读标志 */
	unsigned char s_dirt;		/* 脏标志 */
};

/* 目录项结构 */
struct dir_entry {
	unsigned short inode;		/* inode号 */
	char name[NAME_LEN];		/* 文件名 */
};

/* 全局变量声明 */
extern struct m_inode inode_table[NR_INODE];	/* inode表 */
extern struct file file_table[NR_FILE];		/* 文件表 */
extern struct super_block super_block[NR_SUPER]; /* 超级块表 */
extern struct buffer_head * start_buffer;	/* 缓冲区起始地址 */
extern int nr_buffers;				/* 缓冲区数量 */

/* 函数声明 */
extern void truncate(struct m_inode * inode);		/* 截断文件 */
extern void sync_inodes(void);				/* 同步inode */
extern void wait_on(struct m_inode * inode);		/* 等待inode */
extern int bmap(struct m_inode * inode,int block);	/* 块映射 */
extern int create_block(struct m_inode * inode,int block); /* 创建块 */
extern struct m_inode * namei(const char * pathname);	/* 根据路径名查找inode */
extern int open_namei(const char * pathname, int flag, int mode,
	struct m_inode ** res_inode);			/* 打开文件时查找inode */
extern void iput(struct m_inode * inode);		/* 释放inode */
extern struct m_inode * iget(int dev,int nr);		/* 获取指定设备和编号的inode */
extern struct m_inode * get_empty_inode(void);		/* 获取空闲inode */
extern struct m_inode * get_pipe_inode(void);		/* 获取管道inode */
extern struct buffer_head * get_hash_table(int dev, int block); /* 获取哈希表中的缓冲区 */
extern struct buffer_head * getblk(int dev, int block);	/* 获取指定块的缓冲区 */
extern void ll_rw_block(int rw, struct buffer_head * bh); /* 低级读写块 */
extern void brelse(struct buffer_head * buf);		/* 释放缓冲区 */
extern struct buffer_head * bread(int dev,int block);	/* 读取指定块 */
extern int new_block(int dev);				/* 分配新块 */
extern void free_block(int dev, int block);		/* 释放块 */
extern struct m_inode * new_inode(int dev);		/* 创建新inode */
extern void free_inode(struct m_inode * inode);		/* 释放inode */

extern void mount_root(void);				/* 安装根文件系统 */

/* 内联函数：获取超级块 */
extern inline struct super_block * get_super(int dev)
{
	struct super_block * s;

	for(s = 0+super_block;s < NR_SUPER+super_block; s++)
		if (s->s_dev == dev)
			return s;
	return (struct super_block *)0;
}

#endif