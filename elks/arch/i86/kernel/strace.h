/* ELKS (version >= 0.0.49) system call layout.
 * 
 * syscall_info table format : 
 *
 * { "sys_call_name", # of parameters, \
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

#include <linuxmt/strace.h>

#define packinfo(n, a, b, c) (unsigned)(n | (a << 4) | (b << 8) | (c << 12))

struct syscall_info elks_table[] = {
    { "call",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "exit",		packinfo(1, P_SSHORT, P_NONE,    P_NONE   ) },
    { "fork",		packinfo(0, P_NONE,   P_NONE,    P_NONE   ) },
    { "read",		packinfo(3, P_USHORT, P_PDATA,   P_USHORT ) },
    { "write",		packinfo(3, P_USHORT, P_PDATA,   P_USHORT ) },
    { "open",		packinfo(3, P_PSTR,   P_SSHORT,  P_SSHORT ) },
    { "close",		packinfo(1, P_USHORT, P_NONE,    P_NONE   ) },
    { "wait4",		packinfo(3, P_USHORT, P_PSSHORT, P_SSHORT ) },
    { "creat",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "link",		packinfo(2, P_PSTR,   P_PSTR,    P_NONE   ) },
    { "unlink",		packinfo(1, P_PSTR,   P_NONE,    P_NONE   ) },
    { "execve",		packinfo(3, P_PSTR,   P_PDATA,   P_USHORT ) },
    { "chdir",		packinfo(1, P_PSTR,   P_NONE,    P_NONE   ) },
    { "time",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "mknod",		packinfo(3, P_PSTR,   P_USHORT,  P_USHORT ) },
    { "chmod",		packinfo(2, P_PSTR,   P_USHORT,  P_NONE   ) },
    { "chown",		packinfo(3, P_PSTR,   P_USHORT,  P_USHORT ) },
    { "brk",		packinfo(1, P_USHORT, P_NONE,    P_NONE   ) },
    { "stat",		packinfo(2, P_PSTR,   P_PDATA,   P_NONE   ) },
    { "lseek",		packinfo(3, P_USHORT, P_PDATA,   P_USHORT ) },
    { "getpid",		packinfo(0, P_NONE,   P_NONE,    P_NONE   ) },
    { "mount",		packinfo(3, P_PSTR,   P_PSTR,    P_PSTR   ) },
    { "umount",		packinfo(1, P_PSTR,   P_NONE,    P_NONE   ) },
    { "setuid",		packinfo(1, P_USHORT, P_NONE,    P_NONE   ) },
    { "getuid",		packinfo(0, P_NONE,   P_NONE,    P_NONE   ) },
    { "stime",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "ptrace",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "alarm",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "fstat",		packinfo(2, P_USHORT, P_PDATA,   P_NONE   ) },
    { "pause",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "utime",		packinfo(2, P_PSTR,   P_PDATA,   P_NONE   ) },
    { "chroot",		packinfo(1, P_PSTR,   P_NONE,    P_NONE   ) },
    { "vfork",		packinfo(0, P_NONE,   P_NONE,    P_NONE   ) },
    { "access",		packinfo(2, P_PSTR,   P_USHORT,  P_NONE   ) },
    { "nice",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "sleep",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "sync",		packinfo(0, P_NONE,   P_NONE,    P_NONE   ) },
    { "kill",		packinfo(2, P_USHORT, P_USHORT,  P_NONE   ) },
    { "rename",		packinfo(2, P_PSTR,   P_PSTR,    P_NONE   ) },
    { "mkdir",		packinfo(2, P_PSTR,   P_USHORT,  P_NONE   ) },
    { "rmdir",		packinfo(1, P_PSTR,   P_NONE,    P_NONE   ) },
    { "dup",		packinfo(1, P_USHORT, P_NONE,    P_NONE   ) },
    { "pipe",		packinfo(1, P_PDATA,  P_NONE,    P_NONE   ) },
    { "times",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "profil",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "dup2",		packinfo(2, P_USHORT, P_USHORT,  P_NONE   ) },
    { "setgid",		packinfo(1, P_USHORT, P_NONE,    P_NONE   ) },
    { "getgid",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "signal",         packinfo(2, P_USHORT, P_PDATA,   P_NONE   ) },
    { "getinfo",	packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "fcntl",		packinfo(3, P_USHORT, P_USHORT,  P_USHORT ) },
    { "acct",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "phys",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "lock",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "ioctl",		packinfo(3, P_USHORT, P_USHORT,  P_USHORT ) },
    { "reboot",		packinfo(3, P_USHORT, P_USHORT,  P_SSHORT ) },
    { "mpx",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "lstat",		packinfo(2, P_PSTR,   P_PDATA,   P_NONE   ) },
    { "symlink",	packinfo(2, P_PSTR,   P_PSTR,    P_NONE   ) },
    { "readlink",	packinfo(3, P_PSTR,   P_PDATA,   P_USHORT ) },
    { "umask",		packinfo(1, P_USHORT, P_NONE,    P_NONE   ) },
    { "settimeofday",	packinfo(2, P_PDATA,  P_PDATA,   P_NONE   ) },
    { "gettimeofday",	packinfo(2, P_PDATA,  P_PDATA,   P_NONE   ) },
    { "select",		packinfo(5, P_SSHORT, P_PDATA,   P_PDATA  ) },
    { "readdir",	packinfo(3, P_USHORT, P_PDATA,   P_USHORT ) },
    { "insmod",		packinfo(9, P_NONE,   P_NONE,    P_NONE   ) },
    { "fchown",		packinfo(3, P_USHORT, P_USHORT,  P_USHORT ) },
    { "dlload",		packinfo(2, P_DATA,   P_DATA,    P_NONE   ) },
    { "setsid",		packinfo(0, P_NONE,   P_NONE,    P_NONE   ) },
    { "socket",		packinfo(3, P_SSHORT, P_SSHORT,  P_SSHORT ) },
    { "bind",		packinfo(3, P_SSHORT, P_PDATA,   P_SSHORT ) },
    { "listen",		packinfo(2, P_SSHORT, P_SSHORT,  P_NONE   ) },
    { "accept",		packinfo(3, P_SSHORT, P_DATA,    P_PSSHORT) },
    { "connect",	packinfo(3, P_SSHORT, P_PDATA,   P_SSHORT ) },
#ifdef CONFIG_SYS_VERSION
    { "knlvsn",		packinfo(1, P_PSTR,   P_NONE,    P_NONE   ) },
#endif
};

#endif
