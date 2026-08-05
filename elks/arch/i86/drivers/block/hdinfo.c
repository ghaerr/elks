#include <linuxmt/config.h>
#include <linuxmt/genhd.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>

int ioctl_hdio_geometry(struct gendisk *hd, kdev_t dev, struct hd_geometry *loc)
{
    unsigned short minor = MINOR(dev);
    int drive = minor >> hd->minor_shift;
    struct drive_infot *drivep;
    int err;

    drivep = &hd->drive_info[drive];
    err = verify_area(VERIFY_WRITE, (void *)loc, sizeof(struct hd_geometry));
    if (!err) {
        put_user_char(drivep->heads, &loc->heads);
        put_user_char(drivep->sectors, &loc->sectors);
        put_user(drivep->cylinders, &loc->cylinders);
        put_user_long(hd->part[minor].start_sect, &loc->start);
    }
    return err;
}

void GENPROC show_drive_info(struct drive_infot *drivep, const char *name, int drive,
    int count, const char *eol)
{
    unsigned long size;
    char *unit;
    static char UNITS[4] = "KMGT";

    for (; count; count--, drive++) {
        if (drivep->cylinders != 0) {
            unit = UNITS;
            size = (unsigned long)drivep->sectors * 5;  /* 0.1 kB units */
            if (drivep->sector_size == 1024)
                size <<= 1;
            size *= ((unsigned long) drivep->cylinders) * drivep->heads;

            /* Select appropriate unit */
            while (size > 99999 && unit[1]) {
                debug("DBG: Size = %lu (%X/%X)\n", size, *unit, unit[1]);
                size += 512;
                size /= 1024U;
                unit++;
            }
            debug("DBG: Size = %lu (%X/%X)\n",size,*unit,unit[1]);
            printk("%s%c: %4lu%c CHS %3u,%2d,%d",
                name, drive + (drivep->fdtype < 0? 'a' : '0'), (size/10), *unit,
                drivep->cylinders, drivep->heads, drivep->sectors);
#ifdef CONFIG_ARCH_IBMPC
            /* IBM BIOS INT 13 can't use HD drives > 528MB, must use ATA/CF driver */
            if (drivep->cylinders > 1024 && name[0] == 'h')
                printk(" (CYL > 1024, must use /dev/cf%c)", drive+'a');
#endif
            printk("%s", eol);
        }
        drivep++;
    }
}
