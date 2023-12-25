/*
 * bios.c - ELKS BIOS routines for floppy and hard disk handling
 *
 * Separated from bioshd.c for disk API in biosparms.h
 */
#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/genhd.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/major.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/debug.h>
#include <arch/system.h>
#include "bioshd.h"

#define RESET_DISK_CHG  0       /* =1 to reset BIOS on drive change fixes QEMU retry */

/*
 * Indices for fd_types array. Note these match the value returned
 * by the BIOS in BL less 1, for INT 13h AH=8 (Get Disk Parms) for IBM PC.
 */
#define FD360           0
#define FD1200          1
#define FD720           2
#define FD1440          3
#define FD2880          4       /* QEMU returns 5 */
#define FD2880_DUP      5       /* Dosbox returns 6 */
#define FD1232          6       /* PC/98 only, not returned */

struct drive_infot fd_types[] = {   /* AT/PS2 BIOS reported floppy formats*/
    {40,  9, 2, 512, 0},            /* 360k */
    {80, 15, 2, 512, 1},            /* 1.2M */
    {80,  9, 2, 512, 2},            /* 720k */
    {80, 18, 2, 512, 3},            /* 1.4M */
    {80, 36, 2, 512, 4},            /* 2.88M */
    {80, 36, 2, 512, 5},            /* 2.88M */
    {77,  8, 2, 1024,6},            /* 1.232M PC/98 only */
};

/* BIOS drive mappings */
#ifdef CONFIG_ARCH_PC98
unsigned char bios_drive_map[MAX_DRIVES] = {
    0xA0, 0xA1, 0xA2, 0xA3,             /* hda, hdb */
#ifdef CONFIG_IMG_FD1232
    0x90, 0x91, 0x92, 0x93              /* fd0, fd1 */
#else
    0x30, 0x31, 0x32, 0x33              /* fd0, fd1 */
#endif
};
#else
unsigned char bios_drive_map[MAX_DRIVES] = {
    0x80, 0x81, 0x82, 0x83,             /* hda, hdb */
    0x00, 0x01, 0x02, 0x03              /* fd0, fd1 */
};
#endif

static struct biosparms bdt;

#define SPT             4       /* DDPT offset of sectors per track*/
static unsigned char DDPT[14];  /* our copy of diskette drive parameter table*/
static unsigned long __far *vec1E = _MK_FP(0, 0x1E << 2);

/* As far as I can tell this doesn't actually work, but we might
 * as well try it -- Some XT controllers are happy with it.. [AC]
 */

void bios_disk_reset(int drive)
{
#ifdef CONFIG_ARCH_PC98
    BD_AX = BIOSHD_RESET | drive;
#else
    BD_AX = BIOSHD_RESET;
    BD_DX = drive;
#endif
    call_bios(&bdt);
    /* ignore errors with carry set*/
}

int bios_disk_rw(unsigned cmd, unsigned num_sectors, unsigned drive,
        unsigned cylinder, unsigned head, unsigned sector, unsigned seg, unsigned offset)
{
#ifdef CONFIG_ARCH_PC98
    BD_AX = cmd | drive;
    if (((0xF0 & drive) == 0x80) || ((0xF0 & drive) == 0xA0)) {
        BD_BX = (unsigned int) (num_sectors << 9);
        BD_CX = cylinder;
        BD_DX = (head << 8) | ((sector - 1) & 0xFF);
    }
    else {
        if ((0xF0 & drive) == 0x90) {
            BD_BX = (unsigned int) (num_sectors << 10);
            BD_CX = (3 << 8) | cylinder;
        }
        else {
            BD_BX = (unsigned int) (num_sectors << 9);
            BD_CX = (2 << 8) | cylinder;
        }
        BD_DX = (head << 8) | sector;
    }
    BD_ES = seg;
    BD_BP = offset;
#else

#if RESET_DISK_CHG
    static unsigned last = 0;
    if (drive != last) {
        bios_disk_reset(1); /* fixes QEMU retry when switching drive types #1119 */
        last = drive;
    }
#endif
    BD_AX = cmd | num_sectors;
    BD_CX = (unsigned int) ((cylinder << 8) | ((cylinder >> 2) & 0xc0) | sector);
    BD_DX = (head << 8) | drive;
    BD_ES = seg;
    BD_BX = offset;
#endif
    debug_bios("BIOSHD(%x): %s CHS %d/%d/%d count %d\n", drive,
        cmd==BIOSHD_READ? "read": "write",
        cylinder, head, sector, num_sectors);
#ifdef IODELAY
    /* emulate floppy delay for QEMU */
    unsigned long timeout = jiffies + IODELAY*HZ/100;
    while (!time_after(jiffies, timeout)) continue;
#endif
    return call_bios(&bdt);
}

#ifdef CONFIG_BLK_DEV_BHD
/* This function checks to see which hard drives are active and sets up the
 * drive_info[] table for them.  Ack this is darned confusing...
 */
int INITPROC bios_gethdinfo(struct drive_infot *drivep) {
    int drive, ndrives = 0;

#ifdef CONFIG_ARCH_PC98
    int ide_drives = 0;
    int scsi_id;
    int device_type;
    int call_bios_rvalue;

    /* IDE */
    for (drive = 0; drive < 4; drive++) {
        if (peekb(0x55D,0) & (1 << drive)) {
            BD_AX = BIOSHD_MODESET | (drive + 0x80);
            BD_ES = BD_DI = BD_SI = 0;
            call_bios(&bdt);
            bios_drive_map[ide_drives++] = drive + 0x80;
        }
    }
    if (ide_drives > 0)
        printk("bioshd: Detected IDE hd.\n");
    ndrives = ide_drives;

    /* SCSI */
    if (ndrives < 4) {
        for (scsi_id = 0; scsi_id < 7; scsi_id++) {
            BD_AX = BIOSHD_DRIVE_PARMS | (scsi_id + 0xA0);
            BD_ES = BD_DI = BD_SI = 0;
            call_bios_rvalue = call_bios(&bdt);
            if ((call_bios_rvalue == 0) && (BD_DX & 0xff)) {
                BD_AX = BIOSHD_DEVICE_TYPE | (scsi_id + 0xA0);
                BD_ES = BD_DI = BD_SI = 0;
                BD_BX = 0;
                call_bios(&bdt);
                device_type = BD_BX & 0xf; /* device_type = 0 for Hard Disk */
                if (device_type == 0)
                    bios_drive_map[ndrives++] = scsi_id + 0xA0;
            }
            if (ndrives >= 4) break;
        }
    }
    if (ndrives > ide_drives)
        printk("bioshd: Detected SCSI hd.\n");
#else
    BD_AX = BIOSHD_DRIVE_PARMS;
    BD_DX = 0x80;               /* query hard drives only*/
    BD_ES = BD_DI = BD_SI = 0;  /* guard against BIOS bugs*/
    if (!call_bios(&bdt))
        ndrives = BD_DX & 0xff;
    else
        debug_bios("bioshd: get_drive_parms fail on hd\n");
#endif
    if (ndrives > MAX_DRIVES/2)
        ndrives = MAX_DRIVES/2;

    for (drive = 0; drive < ndrives; drive++) {
#ifdef CONFIG_ARCH_PC98
        BD_AX = BIOSHD_DRIVE_PARMS | bios_drive_map[drive];
#else
        BD_AX = BIOSHD_DRIVE_PARMS;
        BD_DX = drive + 0x80;
#endif
        BD_ES = BD_DI = BD_SI = 0;      /* guard against BIOS bugs*/
        if (call_bios(&bdt) == 0) {
#ifdef CONFIG_ARCH_PC98
            drivep->heads = BD_DX >> 8;
            drivep->sectors = BD_DX & 0xff;
            drivep->cylinders = BD_CX;
#else
            drivep->heads = (BD_DX >> 8) + 1;
            drivep->sectors = BD_CX & 0x3f;
            /* NOTE: some BIOS may underreport cylinders by 1*/
            drivep->cylinders = (((BD_CX & 0xc0) << 2) | (BD_CX >> 8)) + 1;
#endif
            drivep->fdtype = -1;
            drivep->sector_size = 512;
            printk("bioshd: hd%c BIOS CHS %u,%d,%d\n", 'a'+drive, drivep->cylinders,
                drivep->heads, drivep->sectors);
        }
#ifdef CONFIG_IDE_PROBE
        if (sys_caps & CAP_HD_IDE) {            /* Normally PC/AT or higher */
            if (!get_ide_data(drive, drivep)) { /* get CHS from the drive itself */
                /* sanity checks already done, accepting data */
                printk("bioshd: hd%c  IDE CHS %d,%d,%d\n", 'a'+drive, drivep->cylinders,
                drivep->heads, drivep->sectors);
            }
        }
#endif
        drivep++;
    }
    return ndrives;
}
#endif

#ifdef CONFIG_BLK_DEV_BFD_HARD
int INITPROC bios_getfdinfo(struct drive_infot *drivep)
{
/*
 * Hard-coded floppy configuration
 * Set below to match your system. For IBM PC, it's set to a two drive system:
 *
 *              720KB as /dev/fd0
 *      and     720KB as /dev/fd1
 *
 * Drive Type   Format
 *      ~~~~~   ~~~~~~
 *        0     360 KB
 *        1     1.2 MB
 *        2     720 KB
 *        3     1.44 MB
 *        4     2.88 MB (QEMU)
 *        5     2.88 MB (Dosbox)
 *        6     1.232 MB (PC/98 1K sectors)
 * ndrives is number of drives in your system (either 0, 1 or 2)
 */

    int ndrives = FD_DRIVES;

#ifdef CONFIG_ARCH_PC98
#if defined(CONFIG_IMG_FD1232)
    drivep[0] = fd_types[FD1232];
    drivep[1] = fd_types[FD1232];
    drivep[2] = fd_types[FD1232];
    drivep[3] = fd_types[FD1232];
#else
    drivep[0] = fd_types[FD1440];
    drivep[1] = fd_types[FD1440];
    drivep[2] = fd_types[FD1440];
    drivep[3] = fd_types[FD1440];
#endif
#endif

#ifdef CONFIG_ARCH_IBMPC
    drivep[0] = fd_types[FD720];
    drivep[1] = fd_types[FD720];
#endif

    return ndrives;
}

#elif defined(CONFIG_BLK_DEV_BFD)

/* use BIOS to query floppy configuration*/
int INITPROC bios_getfdinfo(struct drive_infot *drivep)
{
    int drive, ndrives = 0;

#ifndef CONFIG_ROMCODE
    /*
     * The INT 13h floppy query will fail on IBM XT v1 BIOS and earlier,
     * so default to # drives from the BIOS data area at 0x040:0x0010 (INT 11h).
     */
    unsigned char equip_flags = peekb(0x10, 0x40);
    if (equip_flags & 0x01)
        ndrives = (equip_flags >> 6) + 1;
#endif

#ifdef CONFIG_ARCH_PC98
    for (drive = 0; drive < 4; drive++) {
        if (peekb(0x55C,0) & (1 << drive)) {
#ifdef CONFIG_IMG_FD1232
            bios_drive_map[DRIVE_FD0 + drive] = drive + 0x90;
            *drivep = fd_types[FD1232];
#else
            bios_drive_map[DRIVE_FD0 + drive] = drive + 0x30;
            *drivep = fd_types[FD1440];
#endif
            ndrives++;  /* floppy drive count*/
            drivep++;
        }
    }
#else
    /* Floppy query may fail if not PC/AT */
    BD_AX = BIOSHD_DRIVE_PARMS;
    BD_DX = 0;                          /* query floppies only*/
    BD_ES = BD_DI = BD_SI = 0;          /* guard against BIOS bugs*/
    if (!call_bios(&bdt)) {
        int drives = BD_DX & 0xff;      /* floppy drive count */
        if (!drives && ndrives) {       /* handle Toshiba T1100 BIOS returning 0 drives */
            for (drive = 0; drive < ndrives; drive++) {
                printk("fd%d: default 720k\n", drive);
                *drivep++ = fd_types[FD720];
            }
            return ndrives;
        } else ndrives = drives;
    } else
        printk("fd: no get drive fn, ndrives %d\n", ndrives);

    /* set drive type for floppies*/
    for (drive = 0; drive < ndrives; drive++) {
        /*
         * If type cannot be determined using BIOSHD_DRIVE_PARMS,
         * set drive type to 1.4MM on AT systems, and 360K for XT.
         */
        BD_AX = BIOSHD_DRIVE_PARMS;
        BD_DX = drive;
        BD_ES = BD_DI = BD_SI = 0;      /* guard against BIOS bugs*/
        if (!call_bios(&bdt)) {         /* returns drive type in BL*/
            *drivep = fd_types[(BD_BX & 0xFF) - 1];
        } else {
            int type = (sys_caps & CAP_PC_AT) ? FD1440 : FD360;
            *drivep = fd_types[type];
            printk("fd%d: default %s\n", drive, type? "1440k": "360k");
        }
        drivep++;
    }
#endif
    return ndrives;
}
#endif

/* set our DDPT sectors per track value*/
void bios_set_ddpt(int max_sectors)
{
        DDPT[SPT] = (unsigned char) max_sectors;
}

/* get the diskette drive parameter table from INT 1E and point to our RAM copy of it*/
void bios_copy_ddpt(void)
{
        unsigned long oldvec = *vec1E;

        /* We want to prevent the BIOS from accidentally doing a "multitrack"
         * floppy read --- and wrapping around from one head to the next ---
         * when ELKS only wants to read from a single track.
         *
         * (E.g. if DDPT SPT = 9, and a disk has 18 sectors per track, and we
         * want to read sectors 9--10 from track 0, side 0, then the BIOS may
         * read sector 9 from track 0, side 0, followed by sector 1 from track
         * 0, side 1, which will be wrong.)
         *
         * To prevent this, we set the DDPT SPT field to the actual sector
         * count per track in the detected disk geometry.  The DDPT SPT
         * should never be smaller than the actual SPT, but it can be larger.
         *
         * Rather than issue INT 13h function 8 (Get Drive Parameters, not implemented
         * on IBM XT BIOS v1 and earlier) to get an accurate DDPT, just copy the original
         * DDPT to RAM, where the sectors per track value will be modified before each
         * INT 13h function 2/3 (Read/Write Disk Sectors).
         * Using a patched DDPT also eliminates the need for a seperate fix for #39/#44.
         */
        fmemcpyw(DDPT, _FP_SEG(DDPT), (void *)(unsigned)oldvec, _FP_SEG(oldvec),
                sizeof(DDPT)/2);
        debug_bios("bioshd: DDPT vector %x:%x SPT %d\n", _FP_SEG(oldvec),
            (unsigned)oldvec, DDPT[SPT]);
        *vec1E = (unsigned long)(void __far *)DDPT;
}

#ifdef CONFIG_ARCH_PC98
/* switch device */
void bios_switch_device98(int target, unsigned int device, struct drive_infot *drivep)
{
    bios_drive_map[target + DRIVE_FD0] =
        (device | (bios_drive_map[target + DRIVE_FD0] & 0x0F));
    if (device == 0x30)
        *drivep = fd_types[FD1440];
    else if (device == 0x10)
        *drivep = fd_types[FD720];
    else if (device == 0x90)
        *drivep = fd_types[FD1232];
}
#endif

/* convert a bios drive number to a bioshd dev_t*/
dev_t INITPROC bios_conv_bios_drive(unsigned int biosdrive)
{
    int minor;
    int partition = 0;
    extern int boot_partition;

#ifdef CONFIG_ARCH_PC98
    if (((biosdrive & 0xF0) == 0x80) || ((biosdrive & 0xF0) == 0xA0)) { /* hard drive*/
        for (minor = 0; minor < 4; minor++) {
            if (biosdrive == bios_drive_map[minor]) break;
        }
        if (minor >= 4) minor = 0;
        partition = boot_partition;     /* saved from add_partition()*/
    } else {
        for (minor = 4; minor < 8; minor++) {
            if (biosdrive == bios_drive_map[minor]) break;
        }
        if (minor >= 8) minor = 4;
    }
#else
    if (biosdrive & 0x80) {             /* hard drive*/
        minor = biosdrive & 0x03;
        partition = boot_partition;     /* saved from add_partition()*/
    } else
        minor = (biosdrive & 0x03) + DRIVE_FD0;
#endif

    return MKDEV(BIOSHD_MAJOR, (minor << MINOR_SHIFT) + partition);
}
