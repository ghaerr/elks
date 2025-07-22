#ifndef __ARCH_8086_ATA_H
#define __ARCH_8086_ATA_H

#include <linuxmt/memory.h>

/* place most of this driver in the far text section if possible */
#if defined(CONFIG_FARTEXT_KERNEL) && !defined(__STRICT_ANSI__)
#define ATPROC __far __attribute__ ((far_section, noinline, section (".fartext.at")))
#else
#define ATPROC
#endif

/*
 * Command block register offsets from base I/O address,
 * for standard ATA and XTIDE v1.
 * These offsets are shifted left 1 for XTCF operation,
 * and the offsets have bits 0 and 3 swapped for XTIDE v2.
 */

#define ATA_REG_DATA        0       /* r/w */
#define ATA_REG_ERR         1       /* r   */
#define ATA_REG_FEAT        1       /*   w */
#define ATA_REG_CNT         2       /* r/w */
#define ATA_REG_LBA_LO      3       /* r/w */
#define ATA_REG_LBA_MD      4       /* r/w */
#define ATA_REG_LBA_HI      5       /* r/w */
#define ATA_REG_SELECT      6       /* r/w */
#define ATA_REG_STATUS      7       /* r   */
#define ATA_REG_CMD         7       /*   w */
#define ATA_REG_DATA_HI     8       /* r/w XTIDE only */

/* XTIDE v2 block register offsets (a3/a0 swapped from ATA/XTIDE v1) */
#define XTIDEV2_DATA        0       /* r/w */
#define XTIDEV2_DATA_HI     1       /* r/w */
#define XTIDEV2_CNT         2       /* r/w */
#define XTIDEV2_LBA_MD      4       /* r/w */
#define XTIDEV2_SELECT      6       /* r/w */
#define XTIDEV2_ERR         8       /* r   */
#define XTIDEV2_LBA_LO      10      /* r/w */
#define XTIDEV2_LBA_HI      12      /* r/w */
#define XTIDEV2_STATUS      14      /* r   */
#define XTIDEV2_CMD         14      /*   w */

/* ATA commands */

#define ATA_CMD_READ        0x20
#define ATA_CMD_WRITE       0x30
#define ATA_CMD_ID          0xEC
#define ATA_CMD_FEAT        0xEF

/* ATA status bits */

#define ATA_STATUS_ERR      0x01
#define ATA_STATUS_DRQ      0x08
#define ATA_STATUS_DFE      0x20
#define ATA_STATUS_BSY      0x80

/* ATA identify buffer short offsets */

#define ATA_INFO_GENL       0
#define ATA_INFO_CYLS       1
#define ATA_INFO_HEADS      3
#define ATA_INFO_SECT_SZ    5
#define ATA_INFO_SPT        6
#define ATA_INFO_CAPS       49
#define ATA_INFO_SECT_LO    60
#define ATA_INFO_SECT_HI    61
#define ATA_INFO_VER_MAJ    80
#define ATA_INFO_VER_MIN    81

#define ATA_CAPS_DMA        0x100
#define ATA_CAPS_LBA        0x200

/* ATA subdriver */

#define ATA_SECTOR_SIZE     512

extern int ata_mode;        /* ATA CF driver operating mode, /bootopts xtide= */

int ATPROC ata_reset(void);
struct drive_infot;
int ATPROC ata_init(int drive, struct drive_infot *drivep);
int ATPROC ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);
int ATPROC ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);

#endif /* !__ARCH_8086_ATA_H*/
