#ifndef __ARCH_8086_ATA_H
#define __ARCH_8086_ATA_H

#include <linuxmt/memory.h>

/* ATA register offsets from base I/O address */

#define ATA_REG_DATA       0
#define ATA_REG_ERR        1
#define ATA_REG_FEAT       1
#define ATA_REG_CNT        2
#define ATA_REG_LBA_LO     3
#define ATA_REG_LBA_MD     4
#define ATA_REG_LBA_HI     5
#define ATA_REG_DRVH       6
#define ATA_REG_CMD        7
#define ATA_REG_STATUS     7

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

void ata_reset(void);
sector_t ata_init(unsigned int drive);
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);

#endif /* !__ARCH_8086_ATA_H*/
