#include <errno.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>

// ftime系统调用（未实现）
int sys_ftime()
{
	return -ENOSYS;
}

// mknod系统调用（未实现）
int sys_mknod()
{
	return -ENOSYS;
}

// break系统调用（未实现）
int sys_break()
{
	return -ENOSYS;
}

// mount系统调用（未实现）
int sys_mount()
{
	return -ENOSYS;
}

// umount系统调用（未实现）
int sys_umount()
{
	return -ENOSYS;
}

// ustat系统调用，获取文件系统统计信息
int sys_ustat(int dev,struct ustat * ubuf)
{
	return -1;
}

// ptrace系统调用（未实现）
int sys_ptrace()
{
	return -ENOSYS;
}

// stty系统调用（未实现）
int sys_stty()
{
	return -ENOSYS;
}

// gtty系统调用（未实现）
int sys_gtty()
{
	return -ENOSYS;
}

// rename系统调用（未实现）
int sys_rename()
{
	return -ENOSYS;
}

// prof系统调用（未实现）
int sys_prof()
{
	return -ENOSYS;
}

// 设置组ID
int sys_setgid(int gid)
{
	if (current->euid && current->uid)
		if (current->gid==gid || current->sgid==gid)
			current->egid=gid;
		else
			return -EPERM;
	else
		current->gid=current->egid=gid;
	return 0;
}

// acct系统调用（未实现）
int sys_acct()
{
	return -ENOSYS;
}

// phys系统调用（未实现）
int sys_phys()
{
	return -ENOSYS;
}

// lock系统调用（未实现）
int sys_lock()
{
	return -ENOSYS;
}

// mpx系统调用（未实现）
int sys_mpx()
{
	return -ENOSYS;
}

// ulimit系统调用（未实现）
int sys_ulimit()
{
	return -ENOSYS;
}

// 获取系统时间
int sys_time(long * tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc) {
		verify_area(tloc,4);
		put_fs_long(i,(unsigned long *)tloc);
	}
	return i;
}

// 设置用户ID
int sys_setuid(int uid)
{
	if (current->euid && current->uid)
		if (uid==current->uid || current->suid==current->uid)
			current->euid=uid;
		else
			return -EPERM;
	else
		current->euid=current->uid=uid;
	return 0;
}

// 设置系统时间
int sys_stime(long * tptr)
{
	if (current->euid && current->uid)
		return -1;
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies/HZ;
	return 0;
}

// 获取进程时间统计
int sys_times(struct tms * tbuf)
{
	if (!tbuf)
		return jiffies;
	verify_area(tbuf,sizeof *tbuf);
	put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);
	put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);
	put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime);
	put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime);
	return jiffies;
}

// 设置程序数据段结束地址
int sys_brk(unsigned long end_data_seg)
{
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;
}

/*
 * 这需要一些严格的检查...
 * 我只是没有胃口去做。我也不完全理解会话/进程组等。
 * 让一些懂的人来解释吧。
 */
int sys_setpgid(int pid, int pgid)
{
	int i;

	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = pid;
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && task[i]->pid==pid) {
			if (task[i]->leader)
				return -EPERM;
			if (task[i]->session != current->session)
				return -EPERM;
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

// 获取进程组ID
int sys_getpgrp(void)
{
	return current->pgrp;
}

// 创建新会话
int sys_setsid(void)
{
	if (current->uid && current->euid)
		return -EPERM;
	if (current->leader)
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;
	return current->pgrp;
}

// 获取系统名称信息
int sys_uname(struct utsname * name)
{
	static struct utsname thisname = {
		"linux .0","nodename","release ","version ","machine "
	};
	int i;

	if (!name) return -1;
	verify_area(name,sizeof *name);
	for(i=0;i<sizeof *name;i++)
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);
	return (0);
}

// 设置文件权限掩码
int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}