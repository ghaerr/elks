/* ELKS (version >= 0.0.49) system call layout.
 * 
 * syscall_info table format : 
 *
 * { system call #, "sys_call_name", # of parameters, \
 *		{parameter types, explained, below}}, 
 * 
 * This table needs to be kept synchronised with the syscall.dat file.
 *
 * The following parameter types are available:
 *
 *	P_NONE		No parameter in this position.
 *
 *	P_SCHAR 	Signed character.
 *	P_UCHAR 	Unsigned character.
 *
 *	P_STR		String of characters (signed or unsigned).
 *	P_PSTR		Pointer to string of characters.
 *
 *	P_SSHORT	Signed short integer.
 *	P_USHORT	Unsigned short integer.
 *
 *	P_PSSHORT	Pointer to signed short integer.
 *	P_PUSHORT	Pointer to unsigned short integer.
 *
 *	P_SLONG 	Signed long integer.
 *	P_ULONG 	Unsigned long integer.
 *
 *	P_PSLONG	Pointer to signed long integer.
 *	P_PULONG	Pointer to unsigned long integer.
 *
 *	P_DATA		Generic Data.
 *	P_PDATA		Pointer to Generic Data.
 *
 *	P_POINTER	Generic Pointer.
 *
 * A maximum of three parameters can be defined for each system call.
 */

#ifndef __STRACE_H__
#define __STRACE_H__

struct syscall_info elks_table[] = {
    {  1, "sys_exit",            0, { P_NONE,     P_NONE,     P_NONE      } },
    {  2, "sys_fork",            0, { P_NONE,     P_NONE,     P_NONE      } },
    {  3, "sys_read",            3, { P_USHORT,   P_PDATA,    P_USHORT    } },
    {  4, "sys_write",           3, { P_USHORT,   P_PDATA,    P_USHORT    } },
    {  5, "sys_open",            3, { P_PSTR,     P_SSHORT,   P_SSHORT    } },
    {  6, "sys_close",           1, { P_USHORT,   P_NONE,     P_NONE      } },
    {  7, "sys_wait",            3, { P_USHORT,   P_USHORT,   P_USHORT    } },
    {  8, "nosys_creat",         0, { P_NONE,     P_NONE,     P_NONE      } },
    {  9, "sys_link",            2, { P_PSTR,     P_PSTR,     P_NONE      } },
    { 10, "sys_unlink",          1, { P_PSTR,     P_NONE,     P_NONE      } },
    { 11, "sys_exec",            3, { P_PSTR,     P_PDATA,    P_USHORT    } },
    { 12, "sys_chdir",           1, { P_PSTR,     P_NONE,     P_NONE      } },
    { 13, "nosys_time",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 14, "sys_mknod",           3, { P_PSTR,     P_USHORT,   P_USHORT    } },
    { 15, "sys_chmod",           2, { P_PSTR,     P_USHORT,   P_NONE      } },
    { 16, "sys_chown",           3, { P_PSTR,     P_USHORT,   P_USHORT    } },
    { 17, "sys_brk",             1, { P_USHORT,   P_NONE,     P_NONE      } },
    { 18, "sys_stat",            2, { P_PSTR,     P_PDATA,    P_NONE      } },
    { 19, "sys_lseek",           3, { P_USHORT,   P_PDATA,    P_USHORT    } },
    { 20, "sys_getpid",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 21, "sys_mount",           3, { P_PSTR,     P_PSTR,     P_PSTR      } },
    { 22, "sys_umount",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 23, "sys_setuid",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 24, "sys_getuid",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 25, "nosys_stime",         0, { P_NONE,     P_NONE,     P_NONE      } },
    { 26, "sys_ptrace",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 27, "sys_alarm",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 28, "sys_fstat",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 29, "sys_pause",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 30, "sys_utime",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 31, "sys_chroot",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 32, "sys_vfork",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 33, "sys_access",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 34, "sys_nice",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 35, "sys_sleep",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 36, "sys_sync",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 37, "sys_kill",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 38, "sys_rename",          2, { P_PSTR,     P_PSTR,     P_NONE      } },
    { 39, "sys_mkdir",           2, { P_PSTR,     P_USHORT,   P_NONE      } },
    { 40, "sys_rmdir",           1, { P_PSTR,     P_NONE,     P_NONE      } },
    { 41, "sys_dup",             0, { P_NONE,     P_NONE,     P_NONE      } },
    { 42, "sys_pipe",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 43, "sys_times",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 44, "sys_profil",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 45, "sys_dup2",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 46, "sys_setgid",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 47, "sys_getgid",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 48, "sys_signal",          2, { P_USHORT,   P_PDATA,    P_NONE      } },
    { 49, "sys_getinfo",         0, { P_NONE,     P_NONE,     P_NONE      } },
    { 50, "sys_fcntl",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 51, "sys_acct",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 52, "nosys_phys",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 53, "sys_lock",            0, { P_NONE,     P_NONE,     P_NONE      } },
    { 54, "sys_ioctl",           3, { P_USHORT,   P_USHORT,   P_PULONG    } },
    { 55, "sys_reboot",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 56, "nosys_mpx",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 57, "sys_lstat",           2, { P_PSTR,     P_PDATA,    P_NONE      } },
    { 58, "sys_symlink",         0, { P_NONE,     P_NONE,     P_NONE      } },
    { 59, "sys_readlink",        0, { P_NONE,     P_NONE,     P_NONE      } },
    { 60, "sys_umask",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 61, "sys_settimeofday",    0, { P_NONE,     P_NONE,     P_NONE      } },
    { 62, "sys_gettimeofday",    0, { P_NONE,     P_NONE,     P_NONE      } },
    { 63, "sys_wait4",           0, { P_NONE,     P_NONE,     P_NONE      } },
    { 64, "sys_readdir",         3, { P_USHORT,   P_PDATA,    P_USHORT    } },
    { 65, "nosys_insmod",        1, { P_PSTR,     P_NONE,     P_NONE      } },
    { 66, "sys_fchown",          3, { P_USHORT,   P_USHORT,   P_USHORT    } },
    { 67, "nosys_dlload",        2, { P_DATA,     P_DATA,     P_NONE      } },
    { 68, "sys_setsid",          0, { P_NONE,     P_NONE,     P_NONE      } },
    { 69, "sys_socket",          3, { P_SSHORT,   P_SSHORT,   P_SSHORT    } },
    { 70, "sys_bind",            3, { P_SSHORT,   P_PDATA,    P_SSHORT    } },
    { 71, "sys_listen",          2, { P_SSHORT,   P_SSHORT,   P_NONE      } },
    { 72, "sys_accept",          3, { P_SSHORT,   P_DATA,     P_PSSHORT   } },
    { 73, "sys_connect",         3, { P_SSHORT,   P_PDATA,    P_SSHORT    } },
#ifdef CONFIG_SYS_VERSION
    { 74, "sys_knlvsn",          1, { P_PSTR,     P_NONE,     P_NONE      } },
#endif
    {  0, "no_sys_call",         0, { P_NONE,     P_NONE,     P_NONE      } }
};

#endif
