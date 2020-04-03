#ifndef __LINUXMT_KDEV_T_H
#define __LINUXMT_KDEV_T_H

#define MINORBITS	8
#define MINORMASK	((1<<MINORBITS) - 1)

#ifdef __KERNEL__

#define MAJOR(dev)	((unsigned short int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned short int) ((dev) & MINORMASK))
#define HASHDEV(dev)	(dev)
#define MKDEV(ma,mi)	((kdev_t) (((ma) << MINORBITS) | (mi)))
#define NODEV		MKDEV(0,0)

#include <linuxmt/types.h>

typedef __u16 kdev_t;

extern char *kdevname(kdev_t);	  /* note: returns pointer to static data! */

/* As long as device numbers in the outside world have 16 bits only,
 * we use these conversions.
 */

#define kdev_t_to_nr(dev)	((__u16) dev)
#define to_kdev_t(dev)		((kdev_t) dev)

#else  /* __KERNEL__*/

/* Some programs want their definitions of MAJOR and MINOR and MKDEV
 * from the kernel sources. These must be the externally visible ones.
 */

#define MAJOR(dev)		(((dev) >> MINORBITS))
#define MINOR(dev)		(((dev) & MINORMASK))
#define MKDEV(major,minor)	((major) << MINORBITS | (minor))

#endif

#endif
