/**********************************************************************
 * ELKS Generic ATA functions
 *
 * This code assumes there is only one ATA controller.
 *
 * This code uses LBA addressing for the disks. Any disks without
 * LBA support (they'd need to be pretty old) are simply ignored.
 *
 * All read/write methods check for ERR and DFE (drive-fault err)
 * There is also an internal timeout counter that once you exceed
 * ATA_TIMEOUT loops, the current operation is aborted. This is
 * intended to prevent code hanging on missing or unresponsive ATA
 * interfaces.
 *
 * Caveat emptor: This code is based on the ATA specifications and
 * some common sense.
 *
 * Ferry Hendrikx, June 2025
 **********************************************************************/

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
static void delay_us(uint16_t n)
{
    while (n--) asm("nop");
}


/**********************************************************************
 * ATA functions
 **********************************************************************/

/**
 * send an ATA command to the drive
 */
int ata_cmd(unsigned int drive, unsigned int cmd, unsigned long sector, unsigned char count)
{
    unsigned char select = 0xA0 | (drive << 4) | ((sector >> 24) & 0x0F);
    unsigned char status;
    unsigned int i;


    // send command
    // if we're doing a disk operation, include the LBA flag

    if (count > 0)
        select |= 0x40;

    outb(select, ATA_PORT_DRVH);
    outb(0x00, ATA_PORT_FEAT);
    outb(count, ATA_PORT_CNT);
    outb((unsigned char) (sector),       ATA_PORT_LBA_LO);
    outb((unsigned char) (sector >> 8),  ATA_PORT_LBA_MD);
    outb((unsigned char) (sector >> 16), ATA_PORT_LBA_HI);
    outb(cmd, ATA_PORT_CMD);


    // wait for drive to be not-busy

    i = 0;

    do
    {
        status = inb(ATA_PORT_STATUS);

        // check for no device
        if (status == 0x00)
            return (-ENXIO);

        // check for error
        if (status & ATA_STATUS_ERR)
            return (-EIO);
        if (status & ATA_STATUS_DFE)
            return (-EINVAL);

        // check for timeout
        if (i++ == ATA_TIMEOUT)
            return (-EINVAL);

    } while ((status & ATA_STATUS_BSY) == ATA_STATUS_BSY);


    // we got this far; there were no errors and we didn't timeout.

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
    unsigned int word, i;

    // send command

    if (ata_cmd(drive, ATA_CMD_ID, 0, 0) != 0)
        return (-ENXIO);


    // wait for drive to be ready/error

    i = 0;

    do
    {
        status = inb(ATA_PORT_STATUS);

        // check for error
        if (status & ATA_STATUS_ERR)
            return (-EIO);
        if (status & ATA_STATUS_DFE)
            return (-EINVAL);

        // check for timeout
        if (i++ == ATA_TIMEOUT)
            return (-EINVAL);

    } while ((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ);


    // read data

    for (i = 0; i < ATA_SECTOR_SIZE; i+=2)
    {
        word = inw(ATA_PORT_DATA);

        buf[i+0] = (unsigned char) (word & 0xFF);
        buf[i+1] = (unsigned char) (word >> 8);
    }

    return (0);
}


/**********************************************************************
 * ATA exported functions
 **********************************************************************/

/**
 * reset the ATA interface
 */
void ata_reset(void)
{
    // set nIEN and SRST bits
    outb(0x06, ATA_PORT_CTRL);

    delay_us(10);

    // set nIEN bit (and clear SRST bit)
    outb(0x02, ATA_PORT_CTRL);
}


/**
 * initialise an ATA device
 *
 * drive : physical drive number (0 or 1)
 */
sector_t ata_init(unsigned int drive)
{
    uint16_t buffer[ATA_SECTOR_SIZE/2];

    if (ata_identify(drive, (char *) &buffer) == 0)
    {
        // ATA LBA support?

        if (buffer[49] & 0x200)
        {
            // ATA LBA sector total (MSB << 16, LSB)

            return ((sector_t) buffer[61] << 16 | buffer[60]);
        }
    }

    return (0);
}


/**
 * read from an ATA device
 *
 * drive : physical drive number (0 or 1)
 * sector: sector number
 * buffer: __far pointer to buffer containing 512 bytes of space
 */
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    unsigned char status;
    unsigned int word, i;

    // send command

    if (ata_cmd(drive, ATA_CMD_READ, sector, 1) != 0)
        return (-ENXIO);


    // wait for drive to be ready/error

    i = 0;

    do
    {
        status = inb(ATA_PORT_STATUS);

        // check for error
        if (status & ATA_STATUS_ERR)
            return (-EIO);
        if (status & ATA_STATUS_DFE)
            return (-EINVAL);

        // check for timeout
        if (i++ == ATA_TIMEOUT)
            return (-EINVAL);

    } while ((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ);


    // read data

    for (i = 0; i < ATA_SECTOR_SIZE; i+=2)
    {
        word = inw(ATA_PORT_DATA);

        buffer[i+0] = (word & 0xFF);
        buffer[i+1] = (word >> 8);
    }

    return (1);
}


/**
 * write to an ATA device
 *
 * drive : physical drive number (0 or 1)
 * sector: sector number
 * buffer: __far pointer to buffer containing 512 bytes of data
 */
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    unsigned char status;
    unsigned int word, i;

    // send command

    if (ata_cmd(drive, ATA_CMD_WRITE, sector, 1) != 0)
        return (-ENXIO);


    // wait for drive to be ready/error

    i = 0;

    do
    {
        status = inb(ATA_PORT_STATUS);

        // check for error
        if (status & ATA_STATUS_ERR)
            return (-EIO);
        if (status & ATA_STATUS_DFE)
            return (-EINVAL);

        // check for timeout
        if (i++ == ATA_TIMEOUT)
            return (-EINVAL);

    } while ((status & ATA_STATUS_DRQ) != ATA_STATUS_DRQ);


    // write data

    for (i = 0; i < ATA_SECTOR_SIZE; i+=2)
    {
        word = buffer[i+0] | buffer[i+1] << 8;
        outw(word, ATA_PORT_DATA);
    }


    // wait for drive to be not-busy

    do
    {
        status = inb(ATA_PORT_STATUS);
    } while ((status & ATA_STATUS_BSY) == ATA_STATUS_BSY);


    // check for error

    status = inb(ATA_PORT_STATUS);

    if (status & ATA_STATUS_ERR)
        return (-EIO);
    if (status & ATA_STATUS_DFE)
        return (-EINVAL);

    return (1);
}

