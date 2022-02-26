#ifndef __LINUXMT_GENHD_H
#define __LINUXMT_GENHD_H

#include <linuxmt/types.h>

/*
 * 	genhd.h Copyright (C) 1992 Drew Eckhardt
 *	Generic hard disk header file by  
 * 		Drew Eckhardt
 *
 *		<drew@colorado.edu>
 */

#define CONFIG_MSDOS_PARTITION 1

/* These two have identical behaviour; use the second one if DOS fdisk gets
   confused about extended/logical partitions starting past cylinder 1023. */

#define DOS_EXTENDED_PARTITION 5
#define LINUX_EXTENDED_PARTITION 0x85

struct partition
{
    unsigned char boot_ind;	/* 0x80 - active */
    unsigned char head;		/* starting head */
    unsigned char sector;	/* starting sector */
    unsigned char cyl;		/* starting cylinder */
    unsigned char sys_ind;	/* What partition type */
    unsigned char end_head;	/* end head */
    unsigned char end_sector;	/* end sector */
    unsigned char end_cyl;	/* end cylinder */
    sector_t start_sect;	/* starting sector counting from 0 */
    sector_t nr_sects;		/* nr of sectors in partition */
};

#ifdef CONFIG_ARCH_PC98
struct partition_pc98
{
    unsigned char boot_ind;	/* bootable */
    unsigned char active;	/* active or sleep */
    unsigned int dummy;
    unsigned char ipl_sector;	/* ipl sector */
    unsigned char ipl_head;	/* ipl head */
    unsigned int ipl_cyl;  	/* ipl cylinder */
    unsigned char sector;	/* starting sector */
    unsigned char head;		/* starting head */
    unsigned int cyl;	  	/* starting cylinder */
    unsigned char end_sector;	/* end sector */
    unsigned char end_head;	/* end head */
    unsigned int end_cyl;	/* end cylinder */
    char name[16];
};
#endif

struct hd_struct
{
    sector_t start_sect;
    sector_t nr_sects;
};

struct gendisk
{
    int major;			/* major number of driver */
    char *major_name;		/* name of major driver */
    int minor_shift;		/* number of times minor is shifted to
				 * get real minor */
    int max_p;			/* maximum partitions per device */
    int max_nr;			/* maximum number of real devices */

    void (*init) ();		/* Initialization called before we do our thing */
    struct hd_struct *part;	/* partition table */
    int *sizes;			/* device size in blocks, copied to blk_size[] */
    int nr_real;		/* number of real devices */

    void *real_devices;		/* internal use */
    struct gendisk *next;
};

extern struct gendisk *gendisk_head;	/* linked list of disks */

#endif
