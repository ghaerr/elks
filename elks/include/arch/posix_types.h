#ifndef __ARCH_8086_POSIX_TYPES_H__
#define __ARCH_8086_POSIX_TYPES_H__

#include <arch/irq.h>

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

#undef	__FD_SET
#define __FD_SET(fd,fdsetp) \
                {\
                int     mask, retval,addr=fdsetp;\
						 \
                icli(); addr += fd >> 4;\
                mask = 1 << (fd & 0xf);\
                *(int*)addr |= mask; isti(); \
                }

#undef	__FD_CLR
#define __FD_CLR(fd,fdsetp) \
                {\
                int     mask, retval,addr=fdsetp;\
                                                 \
                icli(); addr += fd >> 4;\
                mask = 1 << (fd & 0xf);\
                *(int*)addr &= ~mask; isti(); \
       	        }

#undef	__FD_ISSET
#define __FD_ISSET(fd,fdsetp) ((((unsigned long *)fdsetp)[0] & (1<<(fd & 0x1f))) != 0)

#undef	__FD_ZERO
#define __FD_ZERO(fdsetp) (((unsigned long *)fdsetp)[0] = 0UL)

#endif
