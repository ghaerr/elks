#ifndef _LINUXMT_POSIX_TYPES_H
#define _LINUXMT_POSIX_TYPES_H

/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

#ifndef NULL
# define NULL		((void *) 0)
#endif

/*
 * This allows for 20 file descriptors: if NR_OPEN is ever grown
 * beyond that you'll have to change this too. For now 20 fd's seem to be
 * enough for ELKS, so hopefully this won't need changing.
 *
 * Note that POSIX wants the FD_CLEAR(fd,fdsetp) defines to be in
 * <sys/time.h> (and thus <linux/time.h>) - but this is a more logical
 * place for them. Solved by having dummy defines in <sys/time.h>.
 */

/*
 * Those macros may have been defined in <gnu/types.h>. But we always
 * use the ones here. 
 */
#undef __NFDBITS
#define __NFDBITS	(8 * sizeof(unsigned long))

#undef __FD_SETSIZE
#define __FD_SETSIZE	20

#undef __FDSET_LONGS
#define __FDSET_LONGS	(__FD_SETSIZE/__NFDBITS + 1)

#undef __FDELT
#define	__FDELT(d)	((d) / __NFDBITS)

#undef __FDMASK
#define	__FDMASK(d)	(1UL << ((d) % __NFDBITS))

typedef unsigned long __kernel_fd_set;

#include <arch/posix_types.h>

#endif /* _LINUXMT_POSIX_TYPES_H */
