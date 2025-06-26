/*
 * ELKS ATA generic routines
 *
 * This set of routines currently assumes there is only one ATA controller.
 *
 * Ferry Hendrikx, June 2025
 */

#include <stdint.h>
#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <arch/ata.h>
#include <arch/io.h>
#include "ata.h"


/**********************************************************************
 * ATA support functions
 **********************************************************************/

/**
 * Delays some amount of time (not exactly microseconds)
 */
static void delay_us(uint16_t n) {
    while (n--) asm("nop");
}

/**
 * reset the ATA interface
 */
void ata_reset(void)
{
    // set nIEN and SRST bits
    outb_p(0x06, ATA_PORT_CTRL);

    delay_us(10);

    // set nIEN bit (and clear SRST bit)
    outb_p(0x02, ATA_PORT_CTRL);
}

/**
 * send an ATA command to the drive
 */
int ata_cmd(unsigned int drive, unsigned int cmd, unsigned long sector, unsigned char count)
{
    //unsigned char select = 0xA0 | (drive << 4) | ((++sector >> 24) & 0x0F);
    unsigned char select = 0xA0 | (drive << 4) | ((sector >> 24) & 0x0F);
    unsigned char status;

    // if we're doing a disk operation, include the LBA flag
    if (count > 0)
        select |= 0x40;

    outb_p(select, ATA_PORT_DRVH);
    outb_p(0x00, ATA_PORT_FEAT);
    outb_p(count, ATA_PORT_CNT);
    outb_p((unsigned char) (sector),       ATA_PORT_LBA_LO);
    outb_p((unsigned char) (sector >> 8),  ATA_PORT_LBA_MD);
    outb_p((unsigned char) (sector >> 16), ATA_PORT_LBA_HI);
    outb_p(cmd, ATA_PORT_CMD);

    // wait for drive to be not-busy

    do
    {
        status = inb_p(ATA_PORT_STATUS);

        // no such device?
        if (status == 0x00)
            return (-EINVAL);

    } while ((status & ATA_STATUS_BSY) == ATA_STATUS_BSY);

    return (0);
}

/**
 * identify an ATA device
 *
 * drive  : physical drive number (0 or 1)
 * *buffer: pointer to buffer containing 512 bytes of space
 */
int ata_identify(unsigned int drive, char *buf)
{
    unsigned char status;
    unsigned int word;
    int i;

    // send command

    if (ata_cmd(drive, ATA_CMD_ID, 0, 0) != 0)
        return (-EINVAL);

    // wait for drive to be ready/error

    do
    {
        status = inb_p(ATA_PORT_STATUS);
        if (status & ATA_STATUS_ERR)
            return (-EINVAL);
    } while ((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ);

    // read data

    for (i = 0; i < ATA_SECTOR_SIZE; i+=2)
    {
        word = inw(ATA_PORT_DATA);

        buf[i+0] = (unsigned char) (word >> 8);
        buf[i+1] = (unsigned char) (word & 0xFF);
    }

    return (0);
}


/**********************************************************************
 * ATA exported functions
 **********************************************************************/

/**
 * initialise an ATA device
 *
 * drive  : physical drive number (0 or 1)
 */
sector_t ata_init(unsigned int drive)
{
    uint16_t buffer[ATA_SECTOR_SIZE/2];

    uint16_t word60;
    uint16_t word61;

    if (ata_identify(drive, (char *) &buffer) == 0)
    {
        word60 = (buffer[60] >> 8) | (buffer[60] << 8);
        word61 = (buffer[61] >> 8) | (buffer[61] << 8);

        // LBA sector total (MSB << 16, LSB)

        return ((sector_t) word61 << 16 | word60);
    }

    return (0);
}

/**
 * read from an ATA device
 *
 * drive  : physical drive number (0 or 1)
 * sector : sector number
 * *buffer: __far pointer to buffer containing 512 bytes of space
 */
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    unsigned char status;
    unsigned int word;
    int i;

    printk("ata_read drive=%d sector=%lu\n", drive, sector);

    // send command

    if (ata_cmd(drive, ATA_CMD_READ, sector, 1) != 0)
        return (-EINVAL);

    printk("ata_read 1\n");

    // wait for drive to be ready/error

    do
    {
        status = inb_p(ATA_PORT_STATUS);
        if (status & ATA_STATUS_ERR)
            return (-EINVAL);
    } while ((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ);

    printk("ata_read 2\n");

    // read data

    for (i = 0; i < ATA_SECTOR_SIZE; i+=2)
    {
        word = inw(ATA_PORT_DATA);

        buffer[i+0] = (word >> 8);
        buffer[i+1] = (word & 0xFF);
    }

    return (1);
}

/**
 * write to an ATA device
 *
 * drive  : physical drive number (0 or 1)
 * sector : sector number
 * *buffer: __far pointer to buffer containing 512 bytes of data
 */
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    unsigned char status;
    unsigned int word;
    int i;

    // send command

    if (ata_cmd(drive, ATA_CMD_WRITE, sector, 1) != 0)
        return (-EINVAL);

    // wait for drive to be ready/error

    do
    {
        status = inb_p(ATA_PORT_STATUS);
        if (status & ATA_STATUS_ERR)
            return (-EINVAL);
    } while ((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ);

    // write data

    for (i = 0; i < ATA_SECTOR_SIZE; i+=2)
    {
        word = buffer[i+0] << 8 | buffer[i+1];

        outw(ATA_PORT_DATA, word);
    }

    return (1);
}

