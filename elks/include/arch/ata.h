#ifndef __ARCH_8086_ATA_H
#define __ARCH_8086_ATA_H

/* ATA ports */

#ifdef CONFIG_ARCH_IBMPC

#define ATA_PORT_DATA   0x1F0
#define ATA_PORT_ERR    0x1F1
#define ATA_PORT_FEAT   0x1F1
#define ATA_PORT_CNT    0x1F2
#define ATA_PORT_LBA_LO 0x1F3
#define ATA_PORT_LBA_MD 0x1F4
#define ATA_PORT_LBA_HI 0x1F5
#define ATA_PORT_DRVH   0x1F6
#define ATA_PORT_CMD    0x1F7
#define ATA_PORT_STATUS 0x1F7
#define ATA_PORT_CTRL   0x3F6

#endif

#ifdef CONFIG_ARCH_SOLO86

#define ATA_PORT_DATA   0x40
#define ATA_PORT_ERR    0x42
#define ATA_PORT_FEAT   0x42
#define ATA_PORT_CNT    0x44
#define ATA_PORT_LBA_LO 0x46
#define ATA_PORT_LBA_MD 0x48
#define ATA_PORT_LBA_HI 0x4A
#define ATA_PORT_DRVH   0x4C
#define ATA_PORT_CMD    0x4E
#define ATA_PORT_STATUS 0x4E
#define ATA_PORT_CTRL   0x5C

#endif

/* ATA commands */

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_ID      0xEC

/* ATA status bits */

#define ATA_STATUS_ERR  0x01
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_DFE  0x20
#define ATA_STATUS_BSY  0x80

/* ATA identify drive info buffer offsets */
#define ATA_INFO_CYLINDERS  1
#define ATA_INFO_HEADS      3
#define ATA_INFO_SECTSIZE   5
#define ATA_INFO_SPT        6
#define ATA_INFO_CAPS       49
#define ATA_INFO_SECTORS_LO 60
#define ATA_INFO_SECTORS_HI 61

#endif /* !__ARCH_8086_ATA_H*/
