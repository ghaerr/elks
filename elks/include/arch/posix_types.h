#ifndef __ARCH_8086_POSIX_TYPES_H
#define __ARCH_8086_POSIX_TYPES_H

#ifdef __KERNEL__
#include <arch/bitops.h>
#endif

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

#undef  __FD_SET
#ifndef __KERNEL__
#define __FD_SET(fd,fdsetp) (*(fdsetp) |= (__u32)1 << (fd))
#else
#define __FD_SET(fd,fdsetp) set_bit(fd, fdsetp)
#endif

#undef  __FD_CLR
#ifndef __KERNEL__
#define __FD_CLR(fd,fdsetp) (*(fdsetp) &= ~((__u32)1 << (fd)))
#else
#define __FD_CLR(fd,fdsetp) clear_bit(fd, fdsetp)
#endif

#undef  __FD_ISSET
#ifndef __KERNEL__
#define __FD_ISSET(fd,fdsetp) ((*(fdsetp) >> (fd)) & 1)
#else
#define __FD_ISSET(fd,fdsetp) test_bit(fd, fdsetp)
#endif

#undef  __FD_ZERO
#define __FD_ZERO(fdsetp)   (*(fdsetp) = 0UL)

#endif
