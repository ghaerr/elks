#ifndef __LINUXMT_KDEV_T_H
#define __LINUXMT_KDEV_T_H

/* Some programs want their definitions of MAJOR and MINOR and MKDEV
 * from the kernel sources. Here we use unsigned short instead of __u16.
 */
#define MINORBITS           8
#define MINORMASK           ((1 << MINORBITS) - 1)
#define MAJOR(dev)          ((unsigned short) ((dev) >> MINORBITS))
#define MINOR(dev)          ((unsigned short) ((dev) & MINORMASK))
#define MKDEV(major,minor)  ((unsigned short) (((major) << MINORBITS) | (minor)))
#define NODEV               MKDEV(0,0)

#ifdef __KERNEL__
#include <arch/types.h>

typedef __u16               kdev_t;

/* As long as device numbers in the outside world have 16 bits only,
 * we use these conversions.
 */

#define kdev_t_to_nr(dev)   ((__u16) dev)
#define to_kdev_t(dev)      ((kdev_t) dev)

#endif /* __KERNEL__*/

#endif /* __LINUXMT_KDEV_T_H */
