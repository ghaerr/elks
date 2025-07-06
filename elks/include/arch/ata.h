#ifndef __ARCH_8086_ATA_H
#define __ARCH_8086_ATA_H

/* ATA ports */

#ifdef CONFIG_ARCH_IBMPC

#define ATA_BASE_ADDR   0x1F0 //XT-IDE usually uses 0x300

#define ATA_PORT_DATA   (ATA_BASE_ADDR + 0) 
#define ATA_PORT_ERR    (ATA_BASE_ADDR + 1)  
#define ATA_PORT_FEAT   (ATA_BASE_ADDR + 1)
#define ATA_PORT_CNT    (ATA_BASE_ADDR + 2)
#define ATA_PORT_LBA_LO (ATA_BASE_ADDR + 3)
#define ATA_PORT_LBA_MD (ATA_BASE_ADDR + 4)
#define ATA_PORT_LBA_HI (ATA_BASE_ADDR + 5)
#define ATA_PORT_DRVH   (ATA_BASE_ADDR + 6)
#define ATA_PORT_CMD    (ATA_BASE_ADDR + 7)
#define ATA_PORT_STATUS (ATA_BASE_ADDR + 7)
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
#define ATA_CMD_FEAT    0xEF

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
