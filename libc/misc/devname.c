#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include __SYSINC__(devnum.h)

/*
 * Convert a block or character device number to name.
 *
 * This version uses known device numbers to avoid disk reads
 * for floppy systems.
 */

#define ARRAYLEN(a)     (sizeof(a)/sizeof(a[0]))

static struct dev_name_struct {
    char *name;
    mode_t type;
    dev_t num;
} devices[] = {
    /* the 4 partitionable drives must be first */
    { "hda",     S_IFBLK,   DEV_HDA             },
    { "hdb",     S_IFBLK,   DEV_HDB             },
    { "hdc",     S_IFBLK,   DEV_HDC             },
    { "hdd",     S_IFBLK,   DEV_HDD             },
    { "fd0",     S_IFBLK,   DEV_FD0             },
    { "fd1",     S_IFBLK,   DEV_FD1             },
    { "ssd",     S_IFBLK,   MKDEV(SSD_MAJOR, 0) },
    { "rd0",     S_IFBLK,   MKDEV(RAM_MAJOR, 0) },
    { "ttyS0",   S_IFCHR,   DEV_TTYS0           },
    { "ttyS1",   S_IFCHR,   DEV_TTYS1           },
    { "tty1",    S_IFCHR,   DEV_TTY1            },
    { "tty2",    S_IFCHR,   DEV_TTY2            },
    { "tty3",    S_IFCHR,   DEV_TTY3            },
};

static char name[NAME_MAX+6] = "/dev/";

static char *__fast_devname(dev_t dev, mode_t type)
{
    int i;
    unsigned mask;
#define NAMEOFF 5

    for (i = 0; i < ARRAYLEN(devices); i++) {
        mask = (i <= 4)? 0xfff8: 0xffff;
        if (devices[i].type == type && devices[i].num == (dev & mask)) {
            strcpy(&name[NAMEOFF], devices[i].name);
            if (i < 4) {
                if (dev & 0x07) {
                    name[NAMEOFF+3] = '0' + (dev & 7);
                    name[NAMEOFF+4] = '\0';
                }
            }
            return name;
        }
    }
    return NULL;
}

char *devname(dev_t dev, mode_t type)
{
    char *s;

    if ((s = __fast_devname(dev, type)) != NULL)
        return s;
    return "??";
}
