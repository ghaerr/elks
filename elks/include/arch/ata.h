#ifndef __ARCH_8086_ATA_H
#define __ARCH_8086_ATA_H

#include <linuxmt/memory.h>

/* ATA ports */

#ifdef CONFIG_ARCH_IBMPC

/* ATA_BASE_PORT defined as variable in driver (0x1F0 for AT, 0x300 for XT */
/* ATA_CTRL_PORT defined as variable in driver (0x3F6 for AT, 0x308 for XT */

#define ATA_PORT_DATA       (ATA_BASE_PORT + 0x0)
#define ATA_PORT_ERR        (ATA_BASE_PORT + 0x1)
#define ATA_PORT_FEAT       (ATA_BASE_PORT + 0x1)
#define ATA_PORT_CNT        (ATA_BASE_PORT + 0x2)
#define ATA_PORT_LBA_LO     (ATA_BASE_PORT + 0x3)
#define ATA_PORT_LBA_MD     (ATA_BASE_PORT + 0x4)
#define ATA_PORT_LBA_HI     (ATA_BASE_PORT + 0x5)
#define ATA_PORT_DRVH       (ATA_BASE_PORT + 0x6)
#define ATA_PORT_CMD        (ATA_BASE_PORT + 0x7)
#define ATA_PORT_STATUS     (ATA_BASE_PORT + 0x7)

#endif

#ifdef CONFIG_ARCH_SOLO86

#define ATA_BASE_PORT       0x40
#define ATA_CTRL_PORT       0x5C

#define ATA_PORT_DATA       (ATA_BASE_PORT + 0x0)
#define ATA_PORT_ERR        (ATA_BASE_PORT + 0x2)
#define ATA_PORT_FEAT       (ATA_BASE_PORT + 0x2)
#define ATA_PORT_CNT        (ATA_BASE_PORT + 0x4)
#define ATA_PORT_LBA_LO     (ATA_BASE_PORT + 0x6)
#define ATA_PORT_LBA_MD     (ATA_BASE_PORT + 0x8)
#define ATA_PORT_LBA_HI     (ATA_BASE_PORT + 0xA)
#define ATA_PORT_DRVH       (ATA_BASE_PORT + 0xC)
#define ATA_PORT_CMD        (ATA_BASE_PORT + 0xE)
#define ATA_PORT_STATUS     (ATA_BASE_PORT + 0xE)

#endif

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
#define ATA_RETRY           5000        /* # times to poll for not busy */

void ata_reset(void);
sector_t ata_init(unsigned int drive);
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);

#endif /* !__ARCH_8086_ATA_H*/
