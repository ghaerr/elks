#ifndef __LINUXMT_KDEV_T_H
#define __LINUXMT_KDEV_T_H

/* Some programs want their definitions of MAJOR and MINOR and MKDEV
 * from the kernel sources. Here we use unsigned short instead of __u16.
 */
#ifdef __ASSEMBLER__
#define _CAST_U16
#else
#define _CAST_U16   (unsigned short)
#endif

#define MINORBITS           8
#define MINORMASK           ((1 << MINORBITS) - 1)
#define MAJOR(dev)          (_CAST_U16 ((dev) >> MINORBITS))
#define MINOR(dev)          (_CAST_U16 ((dev) & MINORMASK))
#define MKDEV(major,minor)  (_CAST_U16 (((major) << MINORBITS) | (minor)))
#define NODEV               MKDEV(0,0)

#if defined(__KERNEL__) && !defined(__ASSEMBLER__)
#include <arch/types.h>

typedef __u16               kdev_t;

/* As long as device numbers in the outside world have 16 bits only,
 * we use these conversions.
 */

#define kdev_t_to_nr(dev)   ((__u16) dev)
#define to_kdev_t(dev)      ((kdev_t) dev)

#endif /* __KERNEL__*/

#endif /* __LINUXMT_KDEV_T_H */
