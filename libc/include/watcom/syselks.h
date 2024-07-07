/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* 1 Jun 2024 Greg Haerr - syselks.h - modified bld/clib/h/sys386.h for ELKS
*
* Description:  ELKS system calls interface and 8086 specifics.
*
****************************************************************************/


/* On ELKS, syscall return value and error number are both returned
 * in the ax register. Values less than 0 indicate an error (-errno),
 * everything else is a valid return value.
 */

extern int errno;

typedef int syscall_res;

/* macros to access sys_call.. routines return/error value */

#define __syscall_iserror( res )    ((res) < 0)
#define __syscall_errno( res )      (-(res))
#define __syscall_val( type, res )  ((type)(res))

#define __syscall_retcode( res, val )                   \
    if( __syscall_iserror( res ) ) {                    \
        errno = __syscall_errno( res );                 \
        return (syscall_res)(val);                      \
    }                                                   \
    return (syscall_res)res;

#define __syscall_return( type, res )                   \
    __syscall_retcode( res, -1 )                        \
    return( __syscall_val( type, res ) );

#define __syscall_return_pointer( type, res )           \
    __syscall_retcode( res, 0 )                         \
    return( __syscall_val( type, res ) );

/*
 * ELKS system call numbers
 * Must be kept synchronized with elks/arch/i86/kernel/syscall.dat
 */

#define SYS_exit                  1
#define SYS_fork                  2
#define SYS_read                  3
#define SYS_write                 4
#define SYS_open                  5
#define SYS_close                 6
#define SYS_wait4                 7
//#define SYS_creat               8
#define SYS_link                  9
#define SYS_unlink               10
#define SYS_execve               11
#define SYS_chdir                12
#define SYS_time                 13
#define SYS_mknod                14
#define SYS_chmod                15
#define SYS_chown                16
#define SYS_brk                  17
#define SYS_stat                 18
#define SYS_lseek                19
#define SYS_getpid               20
#define SYS_mount                21
#define SYS_umount               22
#define SYS_setuid               23
#define SYS_getuid               24
//#define SYS_stime              25
#define SYS_ptrace               26
#define SYS_alarm                27
#define SYS_fstat                28
//#define SYS_pause              29
#define SYS_utime                30
#define SYS_chroot               31
#define SYS_vfork                32
#define SYS_access               33
//#define SYS_nice               34
//#define SYS_sleep              35
#define SYS_sync                 36
#define SYS_kill                 37
#define SYS_rename               38
#define SYS_mkdir                39
#define SYS_rmdir                40
#define SYS_dup                  41
#define SYS_pipe                 42
//#define SYS_times              43
//#define SYS_prof               44
#define SYS_dup2                 45
#define SYS_setgid               46
#define SYS_getgid               47
#define SYS_signal               48
//#define SYS_getinfo            49
#define SYS_fcntl                50
//#define SYS_acct               51
//#define SYS_phys               52
//#define SYS_lock               53
#define SYS_ioctl                54
#define SYS_reboot               55
//#define SYS_mpx                56
#define SYS_lstat                57
#define SYS_symlink              58
#define SYS_readlink             59
#define SYS_umask                60
#define SYS_settimeofday         61
#define SYS_gettimeofday         62
#define SYS_select               63
#define SYS_readdir              64
//#define SYS_insmod             65
#define SYS_fchown               66
//#define SYS_dllload            67
#define SYS_setsid               68
#define SYS_sbrk                 69
#define SYS_ustatfs              70
#define SYS_setitimer            71
#define SYS_sysctl               72
#define SYS_uname                73

#define SYS_socket              198

#define SYS_bind                200
#define SYS_listen              201
#define SYS_accept              202
#define SYS_connect             203
#define SYS_setsockopt          204
#define SYS_getsocknam          205
#define SYS_fmemalloc           206


#define _sys_exit(rc)       sys_call1n(SYS_exit, rc)

/*
 * Inline assembler for calling Linux system calls
 */

syscall_res sys_call0( unsigned func );
#pragma aux sys_call0 = \
    "int    0x80"       \
    __parm [__ax]       \
    __value [__ax]

syscall_res sys_call1( unsigned func, unsigned r_bx );
#pragma aux sys_call1 = \
    "int    0x80"       \
    __parm [__ax] [__bx]\
    __value [__ax]

void sys_call1n( unsigned func, unsigned r_bx );
#pragma aux sys_call1n =    \
    "int    0x80"           \
    __parm [__ax] [__bx]    \
    __aborts

syscall_res sys_call2( unsigned func, unsigned r_bx, unsigned r_cx );
#pragma aux sys_call2 =         \
    "int    0x80"               \
    __parm [__ax] [__bx] [__cx] \
    __value [__ax]

syscall_res sys_call3( unsigned func, unsigned r_bx, unsigned r_cx, unsigned r_dx );
#pragma aux sys_call3 =                 \
    "int    0x80"                       \
    __parm [__ax] [__bx] [__cx] [__dx]  \
    __value [__ax]

syscall_res sys_call4( unsigned func, unsigned r_bx, unsigned r_cx, unsigned r_dx, unsigned r_di );
#pragma aux sys_call4 =                         \
    "int    0x80"                               \
    __parm [__ax] [__bx] [__cx] [__dx] [__di]   \
    __value [__ax]

syscall_res sys_call5( unsigned func, unsigned r_bx, unsigned r_cx, unsigned r_dx, unsigned r_di, unsigned r_si );
#pragma aux sys_call5 =                              \
    "int    0x80"                                    \
    __parm [__ax] [__bx] [__cx] [__dx] [__di] [__si] \
    __value [__ax]

/* Set the DS register from passed far address before system call */
#if defined(__COMPACT__) || defined(__LARGE__)
#define sys_setseg(ptr)     sys_setds(((unsigned long)ptr) >> 16)
#else
#define sys_setseg(ptr)         /* DS already set in small and medium models */
#endif

void sys_setds(unsigned ds);
#pragma aux sys_setds = \
    __parm [__ds] __modify [__ds];

#if defined(__COMPACT__) || defined(__LARGE__)
#define sys_set_ptr_seg(ptr)    \
    (*((unsigned *)(ptr) + 1) = (unsigned long)(ptr) >> 16)
#else
#define sys_set_ptr_seg(ptr)    /* DS already set in return pointer in small/medium */
#endif
