#ifndef __LINUXMT_GENHD_H
#define __LINUXMT_GENHD_H

#include <linuxmt/types.h>

/*
 *      genhd.h Copyright (C) 1992 Drew Eckhardt
 *      Generic hard disk header file by
 *              Drew Eckhardt
 *
 *              <drew@colorado.edu>
 */

/* These two have identical behaviour; use the second one if DOS fdisk gets
   confused about extended/logical partitions starting past cylinder 1023. */

#define DOS_EXTENDED_PARTITION 5
#define LINUX_EXTENDED_PARTITION 0x85

struct partition
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

struct partition_pc98
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

struct hd_struct
{
    sector_t start_sect;        /* start sector of partition or NOPART */
    sector_t nr_sects;          /* # sectors in partition */
};

struct drive_infot {            /* CHS per drive*/
    unsigned int cylinders;
    int sectors;
    int heads;
    int sector_size;
    int fdtype;                 /* floppy fd_types[] index  or -1 if hd */
};

struct gendisk
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

extern struct drive_infot *last_drive;  /* set to last drivep-> used in read/write */
extern unsigned char bios_drive_map[];  /* map drive to BIOS drivenum */
extern struct drive_infot drive_info[];

#endif
