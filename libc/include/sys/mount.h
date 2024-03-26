#ifndef __SYS_MOUNT_H
#define __SYS_MOUNT_H

#include <features.h>
#include <sys/types.h>
#include __SYSARCHINC__(statfs.h)

/* filesystem types*/
#define FST_MINIX	1
#define FST_MSDOS	2
#define FST_ROMFS	3

/* filesystem mount flags*/
#define MS_RDONLY	    1	/* mount read-only */
#define MS_NOSUID	    2	/* ignore suid and sgid bits */
#define MS_NODEV	    4	/* disallow access to device special files */
#define MS_NOEXEC	    8	/* disallow program execution */
#define MS_SYNCHRONOUS	   16	/* writes are synced at once */
#define MS_REMOUNT	   32	/* alter flags of a mounted FS */
#define MS_AUTOMOUNT	   64	/* auto mount based on superblock */

int mount(const char *dev, const char *dir, int type, int flags);
int umount(const char *dir);

int ustatfs(dev_t dev, struct statfs *statfs, int flags);

int reboot(int magic1, int magic2, int magic3);

#endif
