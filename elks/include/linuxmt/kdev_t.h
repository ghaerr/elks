#ifndef __LINUXMT_KDEV_T_H__
#define __LINUXMT_KDEV_T_H__

#define MINORBITS	8
#define MINORMASK	((1<<MINORBITS) - 1)

#ifdef __KERNEL__

typedef __u16 kdev_t;

#define MAJOR(dev)	((dev) >> MINORBITS)
#define MINOR(dev)	((dev) & MINORMASK)
#define HASHDEV(dev)	(dev)
#define NODEV		0
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))

extern char * kdevname();	/* note: returns pointer to static data! */

/* As long as device numbers in the outside world have 16 bits only,
 * we use these conversions.
 */

#define kdev_t_to_nr(dev)	(dev)
#define to_kdev_t(dev)		(dev)

#else

/* Some programs want their definitions of MAJOR and MINOR and MKDEV
 * from the kernel sources. These must be the externally visible ones.
 */

#define MAJOR(dev)	((dev)>>MINORBITS)
#define MINOR(dev)	((dev) & MINORMASK)
#define MKDEV(ma,mi)	((ma)<<MINORBITS | (mi))

#undef MINORBITS
#undef MINORMASK

#endif
#endif
