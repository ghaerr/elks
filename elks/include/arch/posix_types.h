#ifndef __ARCH_8086_POSIX_TYPES_H
#define __ARCH_8086_POSIX_TYPES_H

#include <arch/irq.h>

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

typedef unsigned short	__kernel_dev_t;
typedef unsigned long	__kernel_ino_t;
typedef unsigned short	__kernel_mode_t;
typedef unsigned short	__kernel_nlink_t;
typedef long		__kernel_off_t;
typedef int		__kernel_pid_t;
typedef unsigned short	__kernel_uid_t;
typedef unsigned short	__kernel_gid_t;
typedef unsigned int	__kernel_size_t;
typedef int		__kernel_ssize_t;
typedef int		__kernel_ptrdiff_t;
typedef long		__kernel_time_t;
typedef long		__kernel_clock_t;
typedef int		__kernel_daddr_t;
typedef char *		__kernel_caddr_t;

#ifdef __GNUC__
typedef long long	__kernel_loff_t;
#endif

typedef struct {
#if defined(__KERNEL__) || defined(__USE_ALL)
	int	val[2];
#else /* !defined(__KERNEL__) && !defined(__USE_ALL) */
	int	__val[2];
#endif /* !defined(__KERNEL__) && !defined(__USE_ALL) */
} __kernel_fsid_t;

#undef	__FD_SET
#define __FD_SET(fd,fdsetp) \
                {\
                int     mask, retval,addr=fdsetp;\
						 \
                icli(); addr += fd >> 3;\
                mask = 1 << (fd & 0xf);\
                *(int*)addr |= mask; isti(); \
                }

#undef	__FD_CLR
#define __FD_CLR(fd,fdsetp) \
                {\
                int     mask, retval,addr=fdsetp;\
                                                 \
                icli(); addr += fd >> 3;\
                mask = 1 << (fd & 0xf);\
                *(int*)addr &= ~mask; isti(); \
       	        }

#undef	__FD_ISSET
#define __FD_ISSET(fd,fdsetp) ((*(int*)((int)fdsetp+fd>>3) & (1<<(fd & 0xf))) != 0)

#undef	__FD_ZERO
#define __FD_ZERO(fdsetp) \
                {\
		  int i = 0; \
		  icli(); for (; i < 32; i++)\
		    ((int*)fdsetp)[i] = 0; isti(); \
		}

#endif /* __ARCH_8086_POSIZ_TYPES_H */
