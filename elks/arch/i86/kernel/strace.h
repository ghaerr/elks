/*
 * ELKS system call table info for strace and kernel stack checking.
 * This table needs to be kept synchronised with the syscall.dat file.
 *
 * strace.h (C) 1997 Chad Page, 2023 Greg Haerr
 *
 * strace allows us to track any system call going into the kernel, and the
 * return value going out.  This include file has the specs for the strace
 * tables in strace.c which will let it produce semi-readable output
 *
 *
 * syscall_info table format : 
 *
 * { "sys_call_name", # of parameters, {parameter types, explained, below}},
 * 
 * The following parameter types are available:
 *
 *      P_NONE          No parameter in this position.
 *
 *      P_SCHAR         Signed character.
 *      P_UCHAR         Unsigned character.
 *
 *      P_STR           String of characters (signed or unsigned).
 *      P_PSTR          Pointer to string of characters.
 *
 *      P_SSHORT        Signed short integer.
 *      P_USHORT        Unsigned short integer.
 *
 *      P_PSSHORT       Pointer to signed short integer.
 *      P_PUSHORT       Pointer to unsigned short integer.
 *
 *      P_SLONG         Signed long integer.
 *      P_ULONG         Unsigned long integer.
 *
 *      P_PSLONG        Pointer to signed long integer.
 *      P_PULONG        Pointer to unsigned long integer.
 *
 *      P_DATA          Generic Data.
 *      P_PDATA         Pointer to Generic Data.
 *
 *      P_POINTER       Generic Pointer.
 *
 * A maximum of three parameters can be defined for each system call.
 *
 * Values organised as follows:
 *
 * Bits  Values  Meaning
 * ~~~~  ~~~~~~  ~~~~~~~
 *  1-0    00    Unsigned
 *         01    Signed
 *         10    Pointer to unsigned
 *         11    Pointer to signed
 *  2+           Data Type
 */

#define P_NONE		  0	/* No parameters                        */
#define P_DATA		  1	/* Generic Data                         */
#define P_POINTER	  2	/* Generic Data Pointer                 */
#define P_PDATA 	  3	/* Pointer to Generic Data              */

#define P_UCHAR 	  4	/* Unsigned Char                        */
#define P_SCHAR 	  5	/* Signed Char                          */
#define P_STR		  6	/* String                               */
#define P_PSTR  	  7	/* Pointer to String                    */

#define P_USHORT	  8	/* Unsigned Short Int                   */
#define P_SSHORT	  9	/* Signed Short Int                     */
#define P_PUSHORT 	 10	/* Pointer to Unsigned Short Int        */
#define P_PSSHORT 	 11	/* Pointer to Signed Short Int          */

#define P_ULONG 	 12	/* Unsigned Long Int                    */
#define P_SLONG 	 13	/* Signed Long Int                      */
#define P_PULONG 	 14	/* Pointer to Unsigned Long Int         */
#define P_PSLONG	 15	/* Pointer to Signed Long Int           */

#define ENTRY(name, info)   { name, info }
#define packinfo(n, a, b, c) (unsigned)(n | (a << 4) | (b << 8) | (c << 12))

struct sc_info {
    const char *s_name;     /* name of syscall */
    unsigned int s_info;    /* # and type of parameters */
};

struct sc_info elks_table1[] = {
    ENTRY("none",           packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 0
    ENTRY("exit",           packinfo(1, P_SSHORT, P_NONE,    P_NONE   )),
    ENTRY("fork",           packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("read",           packinfo(3, P_SSHORT, P_PDATA,   P_USHORT )),
    ENTRY("write",          packinfo(3, P_SSHORT, P_PDATA,   P_USHORT )),
    ENTRY("open",           packinfo(3, P_PSTR,   P_USHORT,  P_USHORT )),
    ENTRY("close",          packinfo(1, P_SSHORT, P_NONE,    P_NONE   )),
    ENTRY("wait4",          packinfo(3, P_SSHORT, P_PSSHORT, P_SSHORT )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 8 creat
    ENTRY("link",           packinfo(2, P_PSTR,   P_PSTR,    P_NONE   )),
    ENTRY("unlink",         packinfo(1, P_PSTR,   P_NONE,    P_NONE   )),   // 10
    ENTRY("execve",         packinfo(3, P_PSTR,   P_PDATA,   P_USHORT )),
    ENTRY("chdir",          packinfo(1, P_PSTR,   P_NONE,    P_NONE   )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 13 time
    ENTRY("mknod",          packinfo(3, P_PSTR,   P_USHORT,  P_USHORT )),
    ENTRY("chmod",          packinfo(2, P_PSTR,   P_USHORT,  P_NONE   )),
    ENTRY("chown",          packinfo(3, P_PSTR,   P_USHORT,  P_USHORT )),
    ENTRY("brk",            packinfo(1, P_POINTER,P_NONE,    P_NONE   )),
    ENTRY("stat",           packinfo(2, P_PSTR,   P_PDATA,   P_NONE   )),
    ENTRY("lseek",          packinfo(3, P_SSHORT, P_PSLONG,  P_USHORT )),
    ENTRY("getpid",         packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 20
    ENTRY("mount",          packinfo(3, P_PSTR,   P_PSTR,    P_PSTR   )),
    ENTRY("umount",         packinfo(1, P_PSTR,   P_NONE,    P_NONE   )),
    ENTRY("setuid",         packinfo(1, P_USHORT, P_NONE,    P_NONE   )),
    ENTRY("getuid",         packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 25 stime
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 26 ptrace
    ENTRY("alarm",          packinfo(1, P_USHORT, P_NONE,    P_NONE   )),
    ENTRY("fstat",          packinfo(2, P_SSHORT, P_PDATA,   P_NONE   )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 29 pause
    ENTRY("utime",          packinfo(2, P_PSTR,   P_PDATA,   P_NONE   )),   // 30
    ENTRY("chroot",         packinfo(1, P_PSTR,   P_NONE,    P_NONE   )),
    ENTRY("vfork",          packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("access",         packinfo(2, P_PSTR,   P_USHORT,  P_NONE   )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 34 nice
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 35 sleep
    ENTRY("sync",           packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("kill",           packinfo(2, P_USHORT, P_USHORT,  P_NONE   )),
    ENTRY("rename",         packinfo(2, P_PSTR,   P_PSTR,    P_NONE   )),
    ENTRY("mkdir",          packinfo(2, P_PSTR,   P_USHORT,  P_NONE   )),
    ENTRY("rmdir",          packinfo(1, P_PSTR,   P_NONE,    P_NONE   )),   // 40
    ENTRY("dup",            packinfo(1, P_SSHORT, P_NONE,    P_NONE   )),
    ENTRY("pipe",           packinfo(1, P_PDATA,  P_NONE,    P_NONE   )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 43 times
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 44 profil
    ENTRY("dup2",           packinfo(2, P_SSHORT, P_USHORT,  P_NONE   )),
    ENTRY("setgid",         packinfo(1, P_USHORT, P_NONE,    P_NONE   )),
    ENTRY("getgid",         packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("signal",         packinfo(2, P_USHORT, P_PDATA,   P_NONE   )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 49 getinfo
    ENTRY("fcntl",          packinfo(3, P_SSHORT, P_USHORT,  P_USHORT )),   // 50
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 51 acct
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 52 phys
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 53 lock
    ENTRY("ioctl",          packinfo(3, P_SSHORT, P_USHORT,  P_USHORT )),
    ENTRY("reboot",         packinfo(3, P_USHORT, P_USHORT,  P_SSHORT )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 56 mpx
    ENTRY("lstat",          packinfo(2, P_PSTR,   P_PDATA,   P_NONE   )),
    ENTRY("symlink",        packinfo(2, P_PSTR,   P_PSTR,    P_NONE   )),
    ENTRY("readlink",       packinfo(3, P_PSTR,   P_PDATA,   P_USHORT )),
    ENTRY("umask",          packinfo(1, P_USHORT, P_NONE,    P_NONE   )),   // 60
    ENTRY("settimeofday",   packinfo(2, P_PDATA,  P_PDATA,   P_NONE   )),
    ENTRY("gettimeofday",   packinfo(2, P_PDATA,  P_PDATA,   P_NONE   )),
    ENTRY("select",         packinfo(5, P_SSHORT, P_PDATA,   P_PDATA  )),
    ENTRY("readdir",        packinfo(3, P_SSHORT, P_PDATA,   P_USHORT )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 65 insmod
    ENTRY("fchown",         packinfo(3, P_SSHORT, P_USHORT,  P_USHORT )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),   // 67 dlload
    ENTRY("setsid",         packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("sbrk",           packinfo(1, P_SSHORT, P_NONE,    P_NONE   )),
    ENTRY("ustatfs",        packinfo(3, P_USHORT, P_PDATA,   P_SSHORT )),   // 70
    ENTRY("setitimer",      packinfo(3, P_SSHORT, P_PDATA,   P_PDATA  )),
    ENTRY("sysctl",         packinfo(3, P_SSHORT, P_STR,     P_SSHORT )),
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("uname",          packinfo(1, P_PDATA,  P_NONE,    P_NONE   )),   // 74
};

#define START_TABLE2  198
struct sc_info elks_table2[] = {
    ENTRY("socket",         packinfo(3, P_SSHORT, P_SSHORT,  P_SSHORT )),   // 198
    ENTRY(0,                packinfo(0, P_NONE,   P_NONE,    P_NONE   )),
    ENTRY("bind",           packinfo(3, P_SSHORT, P_PDATA,   P_USHORT )),   // 200
    ENTRY("listen",         packinfo(2, P_SSHORT, P_SSHORT,  P_NONE   )),
    ENTRY("accept",         packinfo(3, P_SSHORT, P_DATA,    P_PUSHORT)),
    ENTRY("connect",        packinfo(3, P_SSHORT, P_PDATA,   P_SSHORT )),
    ENTRY("setsockopt",     packinfo(5, P_SSHORT, P_SSHORT,  P_SSHORT )), /* +2 args*/
    ENTRY("getsocknam",     packinfo(4, P_SSHORT, P_DATA,    P_PUSHORT)), /* +1 arg*/
    ENTRY("fmemalloc",      packinfo(2, P_USHORT, P_PUSHORT, P_NONE)   ),   // 206
};
