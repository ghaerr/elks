#ifndef __LINUXMT_DEVNUM_H
#define __LINUXMT_DEVNUM_H

#include <linuxmt/major.h>
#include <linuxmt/kdev_t.h>

/*
 * Device numbers for block and character devices
 *
 * NOTE: If a block device MINOR_SHIFT changes, must be updated here also.
 */

/* block devices */
#define DEV_HDA     MKDEV(BIOSHD_MAJOR, 0)
#define DEV_HDB     MKDEV(BIOSHD_MAJOR, 32)
#define DEV_HDC     MKDEV(BIOSHD_MAJOR, 64)
#define DEV_HDD     MKDEV(BIOSHD_MAJOR, 96)
#define DEV_FD0     MKDEV(BIOSHD_MAJOR, 128)
#define DEV_FD1     MKDEV(BIOSHD_MAJOR, 160)
#define DEV_FD2     MKDEV(BIOSHD_MAJOR, 192)
#define DEV_FD3     MKDEV(BIOSHD_MAJOR, 224)
#define DEV_ROM     MKDEV(ROMFLASH_MAJOR, 0)

/* char devices */
#define DEV_TTY1    MKDEV(TTY_MAJOR, 0)
#define DEV_TTY2    MKDEV(TTY_MAJOR, 1)
#define DEV_TTY3    MKDEV(TTY_MAJOR, 2)
#define DEV_TTYS0   MKDEV(TTY_MAJOR, 64)
#define DEV_TTYS1   MKDEV(TTY_MAJOR, 65)


#endif /* !__LINUXMT_DEVNUM_H */
