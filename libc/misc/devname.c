#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <paths.h>
#include <sys/stat.h>
#include <dirent.h>
#include __SYSINC__(devnum.h)
/*
 * Convert a block or character device number to /dev path.
 */

#define USE_FASTVERSION 0   /* =1 to use compiled-in device numbers for floppy speed */

static char path[NAME_MAX+6] = _PATH_DEVSL;     /* /dev/ */

#define NAMEOFF         (sizeof(_PATH_DEVSL) - 1)
#define ARRAYLEN(a)     (sizeof(a)/sizeof(a[0]))

#if USE_FASTVERSION
static struct dev_name_struct {
    char *name;
    mode_t type;
    dev_t num;
} devices[] = {
    /* the 6 partitionable drives must be first */
    { "hda",     S_IFBLK,   DEV_HDA             },
    { "hdb",     S_IFBLK,   DEV_HDB             },
    { "hdc",     S_IFBLK,   DEV_HDC             },
    { "hdd",     S_IFBLK,   DEV_HDD             },
    { "cfa",     S_IFBLK,   DEV_CFA             },
    { "cfb",     S_IFBLK,   DEV_CFB             },
    { "fd0",     S_IFBLK,   DEV_FD0             },
    { "fd1",     S_IFBLK,   DEV_FD1             },
    { "df0",     S_IFBLK,   DEV_DF0             },
    { "df1",     S_IFBLK,   DEV_DF1             },
    { "rom",     S_IFBLK,   DEV_ROM             },
    { "ssd",     S_IFBLK,   MKDEV(SSD_MAJOR, 0) },
    { "rd0",     S_IFBLK,   MKDEV(RAM_MAJOR, 0) },
    { "ttyS0",   S_IFCHR,   DEV_TTYS0           },
    { "ttyS1",   S_IFCHR,   DEV_TTYS1           },
    { "tty1",    S_IFCHR,   DEV_TTY1            },
    { "tty2",    S_IFCHR,   DEV_TTY2            },
    { "tty3",    S_IFCHR,   DEV_TTY3            },
    { "tty4",    S_IFCHR,   DEV_TTY4            },
};

static char *__fast_devname(dev_t dev, mode_t type)
{
    int i;
    unsigned mask;

    for (i = 0; i < ARRAYLEN(devices); i++) {
        mask = (i < 6)? 0xfff8: 0xffff;
        if (devices[i].type == type && devices[i].num == (dev & mask)) {
            strcpy(&path[NAMEOFF], devices[i].name);
            if (i < 6) {
                if (dev & 0x07) {
                    path[NAMEOFF+3] = '0' + (dev & 7);
                    path[NAMEOFF+4] = '\0';
                }
            }
            return path;
        }
    }
    return NULL;
}
#endif

char *devname(dev_t dev, mode_t type)
{
#if USE_FASTVERSION
    char *s = __fast_devname(dev, type);
    if (s)
        return s;
#endif
    DIR *dp;
    struct dirent *d;
    struct stat st;

    dp = opendir(_PATH_DEV);
    if (!dp)
        return NULL;

    while ((d = readdir(dp)) != NULL) {
        if (d->d_name[0] == '.')
            continue;
        strcpy(&path[NAMEOFF], d->d_name);
        if (stat(path, &st) == 0) {
            if ((st.st_mode & S_IFMT) == type && st.st_rdev == dev) {
                closedir(dp);
                return path;
            }
        }
    }
    closedir(dp);
    return NULL;
}
