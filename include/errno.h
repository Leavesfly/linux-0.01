#ifndef _ERRNO_H
#define _ERRNO_H

/*
 * 好吧，由于我没有其他关于可能的错误号的信息来源，
 * 我被迫使用与minix相同的数字。
 * 希望这些是posix或类似的。我不知道（而且posix不告诉我 -
 * 他们想要$$$来获取他们那该死的标准）。
 *
 * 我们不使用minix的_SIGN技巧，所以内核返回值必须自己处理符号。
 *
 * 注意！如果修改此文件，请记得同时修改strerror()！
 */

/* 外部错误号变量 */
extern int errno;

/* 错误码定义 */
#define ERROR		99	/* 通用错误 */
#define EPERM		 1	/* 操作不被允许 */
#define ENOENT		 2	/* 文件或目录不存在 */
#define ESRCH		 3	/* 进程不存在 */
#define EINTR		 4	/* 系统调用被中断 */
#define EIO		 5	/* 输入/输出错误 */
#define ENXIO		 6	/* 设备不存在或设备未就绪 */
#define E2BIG		 7	/* 参数列表过长 */
#define ENOEXEC		 8	/* 可执行文件格式错误 */
#define EBADF		 9	/* 文件描述符错误 */
#define ECHILD		10	/* 子进程不存在 */
#define EAGAIN		11	/* 资源暂时不可用 */
#define ENOMEM		12	/* 内存不足 */
#define EACCES		13	/* 权限不足 */
#define EFAULT		14	/* 地址错误 */
#define ENOTBLK		15	/* 不是块设备 */
#define EBUSY		16	/* 设备或资源忙 */
#define EEXIST		17	/* 文件已存在 */
#define EXDEV		18	/* 跨设备链接 */
#define ENODEV		19	/* 设备不存在 */
#define ENOTDIR		20	/* 不是目录 */
#define EISDIR		21	/* 是目录 */
#define EINVAL		22	/* 无效参数 */
#define ENFILE		23	/* 系统打开文件数过多 */
#define EMFILE		24	/* 打开文件数过多 */
#define ENOTTY		25	/* 不适当的ioctl操作 */
#define ETXTBSY		26	/* 文本文件忙 */
#define EFBIG		27	/* 文件过大 */
#define ENOSPC		28	/* 设备上没有空间 */
#define ESPIPE		29	/* 无效的寻址操作 */
#define EROFS		30	/* 只读文件系统 */
#define EMLINK		31	/* 链接数过多 */
#define EPIPE		32	/* 管道破裂 */
#define EDOM		33	/* 数学参数超出定义域 */
#define ERANGE		34	/* 数学结果不在可表示范围内 */
#define EDEADLK		35	/* 资源死锁 */
#define ENAMETOOLONG	36	/* 文件名过长 */
#define ENOLCK		37	/* 没有可用的锁 */
#define ENOSYS		38	/* 功能未实现 */
#define ENOTEMPTY	39	/* 目录非空 */

#endif