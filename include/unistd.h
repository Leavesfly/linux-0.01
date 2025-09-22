#ifndef _UNISTD_H
#define _UNISTD_H

/* 好吧，这可能是个玩笑，但我正在努力实现它 */
#define _POSIX_VERSION 198808L

#define _POSIX_CHOWN_RESTRICTED	/* 只有root可以执行chown操作（我认为是这样..） */
/* #define _POSIX_NO_TRUNC*/	/* 路径名截断（但请参见内核） */
#define _POSIX_VDISABLE '\0'	/* 禁用功能的字符，如^C */
/*#define _POSIX_SAVED_IDS */	/* 我们会实现这个的 */
/*#define _POSIX_JOB_CONTROL */	/* 我们还没到那一步。很快希望能实现 */

/* 标准输入输出错误文件描述符 */
#define STDIN_FILENO	0	/* 标准输入 */
#define STDOUT_FILENO	1	/* 标准输出 */
#define STDERR_FILENO	2	/* 标准错误 */

#ifndef NULL
#define NULL    ((void *)0)
#endif

/* 访问权限 */
#define F_OK	0	/* 文件存在 */
#define X_OK	1	/* 可执行 */
#define W_OK	2	/* 可写 */
#define R_OK	4	/* 可读 */

/* 文件定位 */
#define SEEK_SET	0	/* 从文件开始处定位 */
#define SEEK_CUR	1	/* 从当前位置定位 */
#define SEEK_END	2	/* 从文件末尾定位 */

/* _SC表示系统配置。我们不经常使用它们 */
#define _SC_ARG_MAX		1
#define _SC_CHILD_MAX		2
#define _SC_CLOCKS_PER_SEC	3
#define _SC_NGROUPS_MAX		4
#define _SC_OPEN_MAX		5
#define _SC_JOB_CONTROL		6
#define _SC_SAVED_IDS		7
#define _SC_VERSION		8

/* 更多（可能）可配置的内容 - 现在是路径名 */
#define _PC_LINK_MAX		1
#define _PC_MAX_CANON		2
#define _PC_MAX_INPUT		3
#define _PC_NAME_MAX		4
#define _PC_PATH_MAX		5
#define _PC_PIPE_BUF		6
#define _PC_NO_TRUNC		7
#define _PC_VDISABLE		8
#define _PC_CHOWN_RESTRICTED	9

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <utime.h>
#include <stddef.h>

#ifdef __LIBRARY__

/* 系统调用号定义 */
#define __NR_setup	0	/* 仅由init使用，用于启动系统 */
#define __NR_exit	1	/* 退出进程 */
#define __NR_fork	2	/* 创建进程 */
#define __NR_read	3	/* 读取文件 */
#define __NR_write	4	/* 写入文件 */
#define __NR_open	5	/* 打开文件 */
#define __NR_close	6	/* 关闭文件 */
#define __NR_waitpid	7	/* 等待子进程 */
#define __NR_creat	8	/* 创建文件 */
#define __NR_link	9	/* 创建硬链接 */
#define __NR_unlink	10	/* 删除文件 */
#define __NR_execve	11	/* 执行程序 */
#define __NR_chdir	12	/* 改变当前目录 */
#define __NR_time	13	/* 获取系统时间 */
#define __NR_mknod	14	/* 创建特殊文件 */
#define __NR_chmod	15	/* 改变文件权限 */
#define __NR_chown	16	/* 改变文件所有者 */
#define __NR_break	17	/* 改变数据段大小 */
#define __NR_stat	18	/* 获取文件状态 */
#define __NR_lseek	19	/* 定位文件读写位置 */
#define __NR_getpid	20	/* 获取进程ID */
#define __NR_mount	21	/* 安装文件系统 */
#define __NR_umount	22	/* 卸载文件系统 */
#define __NR_setuid	23	/* 设置用户ID */
#define __NR_getuid	24	/* 获取用户ID */
#define __NR_stime	25	/* 设置系统时间 */
#define __NR_ptrace	26	/* 进程跟踪 */
#define __NR_alarm	27	/* 设置报警 */
#define __NR_fstat	28	/* 获取文件状态 */
#define __NR_pause	29	/* 暂停进程 */
#define __NR_utime	30	/* 改变文件访问和修改时间 */
#define __NR_stty	31	/* 改变终端设置 */
#define __NR_gtty	32	/* 获取终端设置 */
#define __NR_access	33	/* 检查文件访问权限 */
#define __NR_nice	34	/* 设置进程优先级 */
#define __NR_ftime	35	/* 获取日期和时间 */
#define __NR_sync	36	/* 同步文件系统 */
#define __NR_kill	37	/* 发送信号 */
#define __NR_rename	38	/* 重命名文件 */
#define __NR_mkdir	39	/* 创建目录 */
#define __NR_rmdir	40	/* 删除目录 */
#define __NR_dup	41	/* 复制文件描述符 */
#define __NR_pipe	42	/* 创建管道 */
#define __NR_times	43	/* 获取进程时间 */
#define __NR_prof	44	/* 程序剖析 */
#define __NR_brk	45	/* 改变数据段大小 */
#define __NR_setgid	46	/* 设置组ID */
#define __NR_getgid	47	/* 获取组ID */
#define __NR_signal	48	/* 设置信号处理 */
#define __NR_geteuid	49	/* 获取有效用户ID */
#define __NR_getegid	50	/* 获取有效组ID */
#define __NR_acct	51	/* 进程记账 */
#define __NR_phys	52	/* 物理内存管理 */
#define __NR_lock	53	/* 文件锁定 */
#define __NR_ioctl	54	/* 设备控制 */
#define __NR_fcntl	55	/* 文件控制 */
#define __NR_mpx	56	/* 多路复用 */
#define __NR_setpgid	57	/* 设置进程组ID */
#define __NR_ulimit	58	/* 设置资源限制 */
#define __NR_uname	59	/* 获取系统信息 */
#define __NR_umask	60	/* 设置文件权限掩码 */
#define __NR_chroot	61	/* 改变根目录 */
#define __NR_ustat	62	/* 获取文件系统统计 */
#define __NR_dup2	63	/* 复制文件描述符 */
#define __NR_getppid	64	/* 获取父进程ID */
#define __NR_getpgrp	65	/* 获取进程组ID */
#define __NR_setsid	66	/* 创建会话 */

/* 无参数系统调用宏 */
#define _syscall0(type,name) \
type name(void) \
{ \
type __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name)); \
if (__res >= 0) \
	return __res; \
errno = -__res; \
return -1; \
}

/* 一个参数系统调用宏 */
#define _syscall1(type,name,atype,a) \
type name(atype a) \
{ \
type __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" (a)); \
if (__res >= 0) \
	return __res; \
errno = -__res; \
return -1; \
}

/* 两个参数系统调用宏 */
#define _syscall2(type,name,atype,a,btype,b) \
type name(atype a,btype b) \
{ \
type __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" (a),"c" (b)); \
if (__res >= 0) \
	return __res; \
errno = -__res; \
return -1; \
}

/* 三个参数系统调用宏 */
#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a,btype b,ctype c) \
{ \
type __res; \
__asm__ volatile ("int $0x80" \
	: "=a" (__res) \
	: "0" (__NR_##name),"b" (a),"c" (b),"d" (c)); \
if (__res<0) \
	errno=-__res , __res = -1; \
return __res;\
}

#endif /* __LIBRARY__ */

/* 外部变量 */
extern int errno;	/* 错误号 */

/* 函数声明 */
int access(const char * filename, mode_t mode);		/* 检查文件访问权限 */
int acct(const char * filename);			/* 进程记账 */
int alarm(int sec);					/* 设置报警 */
int brk(void * end_data_segment);			/* 改变数据段大小 */
void * sbrk(ptrdiff_t increment);			/* 改变数据段大小 */
int chdir(const char * filename);			/* 改变当前目录 */
int chmod(const char * filename, mode_t mode);		/* 改变文件权限 */
int chown(const char * filename, uid_t owner, gid_t group); /* 改变文件所有者 */
int chroot(const char * filename);			/* 改变根目录 */
int close(int fildes);					/* 关闭文件 */
int creat(const char * filename, mode_t mode);		/* 创建文件 */
int dup(int fildes);					/* 复制文件描述符 */
int execve(const char * filename, char ** argv, char ** envp); /* 执行程序 */
int execv(const char * pathname, char ** argv);		/* 执行程序 */
int execvp(const char * file, char ** argv);		/* 执行程序 */
int execl(const char * pathname, char * arg0, ...);	/* 执行程序 */
int execlp(const char * file, char * arg0, ...);	/* 执行程序 */
int execle(const char * pathname, char * arg0, ...);	/* 执行程序 */
volatile void exit(int status);				/* 退出进程 */
volatile void _exit(int status);			/* 退出进程 */
int fcntl(int fildes, int cmd, ...);			/* 文件控制 */
int fork(void);						/* 创建进程 */
int getpid(void);					/* 获取进程ID */
int getuid(void);					/* 获取用户ID */
int geteuid(void);					/* 获取有效用户ID */
int getgid(void);					/* 获取组ID */
int getegid(void);					/* 获取有效组ID */
int ioctl(int fildes, int cmd, ...);			/* 设备控制 */
int kill(pid_t pid, int signal);			/* 发送信号 */
int link(const char * filename1, const char * filename2); /* 创建硬链接 */
int lseek(int fildes, off_t offset, int origin);	/* 定位文件读写位置 */
int mknod(const char * filename, mode_t mode, dev_t dev); /* 创建特殊文件 */
int mount(const char * specialfile, const char * dir, int rwflag); /* 安装文件系统 */
int nice(int val);					/* 设置进程优先级 */
int open(const char * filename, int flag, ...);		/* 打开文件 */
int pause(void);					/* 暂停进程 */
int pipe(int * fildes);					/* 创建管道 */
int read(int fildes, char * buf, off_t count);		/* 读取文件 */
int setpgrp(void);					/* 设置进程组 */
int setpgid(pid_t pid,pid_t pgid);			/* 设置进程组ID */
int setuid(uid_t uid);					/* 设置用户ID */
int setgid(gid_t gid);					/* 设置组ID */
void (*signal(int sig, void (*fn)(int)))(int);		/* 设置信号处理 */
int stat(const char * filename, struct stat * stat_buf); /* 获取文件状态 */
int fstat(int fildes, struct stat * stat_buf);		/* 获取文件状态 */
int stime(time_t * tptr);				/* 设置系统时间 */
int sync(void);						/* 同步文件系统 */
time_t time(time_t * tloc);				/* 获取系统时间 */
/* 注意：times函数在<sys/times.h>中声明 */
int ulimit(int cmd, long limit);			/* 设置资源限制 */
mode_t umask(mode_t mask);				/* 设置文件权限掩码 */
int umount(const char * specialfile);			/* 卸载文件系统 */
int uname(struct utsname * name);			/* 获取系统信息 */
int unlink(const char * filename);			/* 删除文件 */
int ustat(dev_t dev, struct ustat * ubuf);		/* 获取文件系统统计 */
int utime(const char * filename, struct utimbuf * times); /* 改变文件访问和修改时间 */
pid_t waitpid(pid_t pid,int * wait_stat,int options);	/* 等待子进程 */
pid_t wait(int * wait_stat);				/* 等待子进程 */
int write(int fildes, const char * buf, off_t count);	/* 写入文件 */
int dup2(int oldfd, int newfd);				/* 复制文件描述符 */
int getppid(void);					/* 获取父进程ID */
pid_t getpgrp(void);					/* 获取进程组ID */
pid_t setsid(void);					/* 创建会话 */

#endif