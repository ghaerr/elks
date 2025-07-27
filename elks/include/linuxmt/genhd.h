#ifndef __LINUXMT_GENHD_H
#define __LINUXMT_GENHD_H

#include <linuxmt/types.h>
#include <linuxmt/kdev_t.h>

/*
 *      genhd.h Copyright (C) 1992 Drew Eckhardt
 *      Generic hard disk header file by
 *              Drew Eckhardt
 *
 *              <drew@colorado.edu>
 */

/* the genhd partition code can be INITPROC if called only at kernel init time */
#if defined(CONFIG_FARTEXT_KERNEL) && !defined(__STRICT_ANSI__)
#define GENPROC __far __attribute__ ((far_section, noinline, section (".fartext.gen")))
#else
#define GENPROC
#endif

/* These two have identical behaviour; use the second one if DOS fdisk gets
   confused about extended/logical partitions starting past cylinder 1023. */
#define DOS_EXTENDED_PARTITION   5
#define LINUX_EXTENDED_PARTITION 0x85

struct partition                /* IBM PC MBR partition entry */
{
    unsigned char boot_ind;     /* 0x80 - active */
    unsigned char head;         /* starting head */
    unsigned char sector;       /* starting sector */
    unsigned char cyl;          /* starting cylinder */
    unsigned char sys_ind;      /* What partition type */
    unsigned char end_head;     /* end head */
    unsigned char end_sector;   /* end sector */
    unsigned char end_cyl;      /* end cylinder */
    sector_t start_sect;        /* starting sector counting from 0 */
    sector_t nr_sects;          /* nr of sectors in partition */
};

struct partition_pc98           /* PC-98 IPL1 partition entry */
{
    unsigned char boot_ind;     /* bootable */
    unsigned char active;       /* active or sleep */
    unsigned int dummy;
    unsigned char ipl_sector;   /* ipl sector */
    unsigned char ipl_head;     /* ipl head */
    unsigned int ipl_cyl;       /* ipl cylinder */
    unsigned char sector;       /* starting sector */
    unsigned char head;         /* starting head */
    unsigned int cyl;           /* starting cylinder */
    unsigned char end_sector;   /* end sector */
    unsigned char end_head;     /* end head */
    unsigned int end_cyl;       /* end cylinder */
    char name[16];
};

#define NOPART      -1UL        /* no partition at start_sect */

struct hd_struct                /* internal partition table entry */
{
    sector_t start_sect;        /* start sector of partition or NOPART */
    sector_t nr_sects;          /* # sectors in partition */
};

#define HARDDISK    (-1)        /* fdtype for hard disk */

struct drive_infot              /* CHS per drive*/
{
    unsigned int cylinders;
    int sectors;
    int heads;
    int sector_size;
    int fdtype;                 /* floppy fd_types[] index  or HARDDISK if hd */
};

struct gendisk                  /* general disk information struct */
{
    int major;                  /* major number of driver */
    const char *major_name;     /* name of major driver */
    int minor_shift;            /* number of times minor is shifted to get real minor */
    int max_partitions;         /* maximum partitions per device */
    int num_drives;             /* maximum number of real devices */
    struct hd_struct *part;     /* partition table */
    int nr_hd;                  /* number of hard drives */
    struct drive_infot *drive_info;
};

struct hd_geometry              /* structure returned from HDIO_GETGEO */
{
    unsigned char heads;
    unsigned char sectors;
    unsigned short cylinders;
    unsigned long start;
};

/* hd/ide ctl's that pass (arg) ptrs to user space are numbered 0x030n/0x031n */
#define HDIO_GETGEO     0x0301  /* get device geometry */

extern struct gendisk bioshd_gendisk;   /* IBM for bios_disk_park_all() */
extern struct drive_infot *last_drive;  /* PC98 set to last drivep-> used in read/write */
extern unsigned char bios_drive_map[];  /* map drive to BIOS drivenum */
extern int boot_partition;              /* MBR boot partition, if any */

void GENPROC init_partitions(struct gendisk *dev);
void GENPROC show_drive_info(struct drive_infot *drivep, const char *name, int drive,
    int count, const char *eol);

int ioctl_hdio_geometry(struct gendisk *hd, kdev_t dev, struct hd_geometry *loc);

#endif
