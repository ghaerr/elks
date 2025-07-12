/**********************************************************************
 * ELKS Generic ATA functions
 *
 * This code assumes there is only one ATA controller.
 *
 * This code uses LBA addressing for the disks. Any disks without
 * LBA support (they'd need to be pretty old) are simply ignored.
 *
 * All read/write methods follow this same process:
 * - wait till drive is not busy
 * - send drive/head information
 * - wait till drive is not busy
 * - send count and sector information
 * - wait till drive is not busy
 * - check ERR, DFE and DRQ bits
 * - read/write data
 * - wait till drive is not busy
 *
 * Caveat emptor: This code is based on the ATA specifications and
 * some common sense.
 *
 * Ferry Hendrikx, June 2025
 **********************************************************************/

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/heap.h>
#include <linuxmt/debug.h>
#include <arch/ata.h>
#include <arch/io.h>
#include "ata.h"

static char use_8bitmode;

#ifdef CONFIG_ARCH_IBMPC
/*
 * The ATA base port is dynamically set on IBM PCs:
 * For 8086 systems, use XT-IDE's port 0x300 (set in ata_reset),
 * otherwise use the standard ATA port 0x1F0.
 */
static unsigned int ata_base_port = 0x1F0;

#define ATA_BASE_PORT   ata_base_port
#endif

/**********************************************************************
 * ATA support functions
 **********************************************************************/

/**
 * ATA delay
 */
static void ata_delay(void)
{
    unsigned short i = 1000;

    while (i--) asm("nop");
}

/**
 * ATA delay 400ns
 */
static void ata_delay_400(void)
{
    unsigned short i;

    for (i = 0; i < 5; i++)
        inb(ATA_PORT_STATUS);
}

/**
 * ATA wait for state
 */
static int ata_wait(unsigned char state)
{
    unsigned char status;
    unsigned int i;

    status = 0;

    // loop while the state is true

    for (i = 0; i < 500; i++)
    {
        status = inb(ATA_PORT_STATUS);

        // are we done?

        if (status & state)
            ata_delay();
        else
            return (0);
    }

    return -ENXIO;
}

/**
 * ATA select
 */
static int ata_select(unsigned int drive, unsigned int cmd, unsigned long sector)
{
    unsigned char select = 0xA0 | 0x40 | (drive << 4) | ((sector >> 24) & 0x0F);
    unsigned char status;
    unsigned int i;

    // wait for drive to be non-busy

    for (i = 0; i < ATA_RETRY; i++)
    {
        status = ata_wait(ATA_STATUS_BSY);

        if (status == 0)
            break;
    }

    if (status)
        return -ENXIO;

    // send

    outb(select, ATA_PORT_DRVH);

    ata_delay_400();

    // wait for drive to be non-busy

    for (i = 0; i < ATA_RETRY; i++)
    {
        status = ata_wait(ATA_STATUS_BSY);

        if (status == 0)
            return 0;
    }

    return -ENXIO;
}


/* read from I/O port into far buffer */
static void read_ioport(int port, unsigned char __far *buffer, size_t count)
{
    size_t i;
    unsigned int word;

    if (use_8bitmode)
    {
        for (i = 0; i < count; i++)
        {
            buffer[i] = inb(port);
        }
    }
    else
    {
        for (i = 0; i < count; i+=2)
        {
            word = inw(port);

            buffer[i+0] = (unsigned char) (word & 0xFF);
            buffer[i+1] = (unsigned char) (word >> 8);
        }
    }
}

/* write from far buffer to I/O port */
static void write_ioport(int port, unsigned char __far *buffer, size_t count)
{
    size_t i;
    unsigned int word;

    if (use_8bitmode)
    {
        for (i = 0; i < count; i++)
        {
            outb(buffer[i], port);
        }
    }
    else
    {
        for (i = 0; i < count; i+=2)
        {
            word = buffer[i+0] | buffer[i+1] << 8;
            outw(word, port);
        }
    }
}

/**********************************************************************
 * ATA functions
 **********************************************************************/

/**
 * ATA set 8-bit transfer mode if possible
 */
static int ata_set8bitmode(void)
{
    int i, status;

    // set 8-bit transfer mode

    outb(0x01, ATA_PORT_FEAT);
    outb(ATA_CMD_FEAT, ATA_PORT_CMD);


    // wait for drive to be not-busy

    for (i = 0; i < ATA_RETRY; i++)
    {
        status = ata_wait(ATA_STATUS_BSY);

        // are we done?

        if (status == 0)
            break;
    }

    if (status)
        return -ENXIO;


    // check for error

    status = inb(ATA_PORT_STATUS);

    if (status & ATA_STATUS_ERR)
    {
        printk("cf: can't set 8-bit transfer mode (error %02xh)\n", inb(ATA_PORT_ERR));
        return -EINVAL;
    }

    printk("cf: setting 8-bit transfer mode\n");
    return 0;
}

/**
 * send an ATA command to the drive
 */
static int ata_cmd(unsigned int drive, unsigned int cmd, unsigned long sector,
    unsigned int count)
{
    unsigned char status;
    unsigned int i;
    int error;


    // send command

    error = ata_select(drive, cmd, sector);
    if (error)
        return error;

    outb(0x00, ATA_PORT_FEAT);
    outb(count, ATA_PORT_CNT);
    outb((unsigned char) (sector),       ATA_PORT_LBA_LO);
    outb((unsigned char) (sector >> 8),  ATA_PORT_LBA_MD);
    outb((unsigned char) (sector >> 16), ATA_PORT_LBA_HI);
    outb(cmd, ATA_PORT_CMD);


    // wait for drive to be not-busy

    for (i = 0; i < ATA_RETRY; i++)
    {
        status = ata_wait(ATA_STATUS_BSY);

        // are we done?

        if (status == 0)
            break;
    }
    if (status)
        return -ENXIO;

    // check for error

    status = inb(ATA_PORT_STATUS);

    if (status & (ATA_STATUS_ERR|ATA_STATUS_DFE))
        return -EIO;

    if (! (status & ATA_STATUS_DRQ))
        return -EINVAL;

    return 0;
}


/**
 * identify an ATA device
 *
 * drive  : physical drive number (0 or 1)
 * *buffer: pointer to buffer containing 512 bytes of space
 */
static int ata_identify(unsigned int drive, unsigned char __far *buf)
{
    int error;


    // send command

    error = ata_cmd(drive, ATA_CMD_ID, 0, 0);
    if (error)
        return 0;

    // read data

    read_ioport(ATA_PORT_DATA, buf, ATA_SECTOR_SIZE);

    return (! ata_wait(ATA_STATUS_BSY));
}


/**********************************************************************
 * ATA exported functions
 **********************************************************************/

/**
 * reset the ATA interface
 */
void ata_reset(void)
{
    unsigned char select = 0xA0;

#ifdef CONFIG_ARCH_IBMPC
    // if not IBM PC/AT+ or later (w/80286), assume XT-IDE port 0x300
    if (arch_cpu < 6)
        ata_base_port = 0x300;
#endif

    // controller reset

    outb(select, ATA_PORT_DRVH);

    ata_delay();

    // set nIEN and SRST bits
    outb(0x06, ATA_PORT_CTRL);

    ata_delay();

    // set nIEN bit (and clear SRST bit)
    outb(0x02, ATA_PORT_CTRL);

    ata_delay();


    // controller 8-bit transfer is requested for 8086/80186 systems:
    // try and turn on 8-bit mode

    use_8bitmode = 0;

    if (arch_cpu < 6)
        use_8bitmode = (ata_set8bitmode() == 0);
}


/**
 * initialise an ATA device
 *
 * drive : physical drive number (0 or 1)
 */
sector_t ata_init(unsigned int drive)
{
    unsigned short *buffer;
    sector_t total = 0;


    // allocate buffer

    buffer = (unsigned short *) heap_alloc(ATA_SECTOR_SIZE, HEAP_TAG_DRVR);

    if (!buffer)
        return 0;


    // identify drive

    if (ata_identify(drive, (unsigned char __far *) buffer))
    {
        // ATA LBA sector total (MSB << 16, LSB)
        total = (sector_t)buffer[ATA_INFO_SECT_HI] << 16 | buffer[ATA_INFO_SECT_LO];

        printk("cf%d: ATA port %x, %luK CHS %2u,%2u,%2u SZ %x ",
            drive, ATA_BASE_PORT, total >> 1,
            buffer[ATA_INFO_CYLS], buffer[ATA_INFO_HEADS],
            buffer[ATA_INFO_SPT], buffer[ATA_INFO_SECT_SZ]);

        // ATA version
        if ((buffer[ATA_INFO_VER_MAJ] != 0) && (buffer[ATA_INFO_VER_MAJ] != 0xFFFF))
        {
            // dump out version word (don't bother decoding it)

            printk("VER %x ", buffer[ATA_INFO_VER_MAJ]);
        }

        // Sanity check
        if ((buffer[ATA_INFO_CYLS] == 0 || buffer[ATA_INFO_CYLS] > 0x7F00) ||
            (buffer[ATA_INFO_HEADS] == 0 || buffer[ATA_INFO_HEADS] > 16) ||
            (buffer[ATA_INFO_SPT] == 0 || buffer[ATA_INFO_SPT] > 63))
        {
            printk("not present");
            //total = 0;
        }
        else if (! (buffer[ATA_INFO_CAPS] & ATA_CAPS_LBA))      // ATA LBA support?
        {
            printk("LBA not detected");
            //total = 0;
        }

        printk("\n");
    }

    heap_free(buffer);

    return total;
}


/**
 * read from an ATA device
 *
 * drive : physical drive number (0 or 1)
 * sector: sector number
 * buf/seg: I/O buffer address
 */
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    int error;


    // send command

    error = ata_cmd(drive, ATA_CMD_READ, sector, 1);
    if (error)
        return 0;


    // read data

    read_ioport(ATA_PORT_DATA, buffer, ATA_SECTOR_SIZE);

    return (!ata_wait(ATA_STATUS_BSY));
}


/**
 * write to an ATA device
 *
 * drive : physical drive number (0 or 1)
 * sector: sector number
 * buf/seg: I/O buffer address
 */
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    int error;


    // send command

    error = ata_cmd(drive, ATA_CMD_WRITE, sector, 1);
    if (error)
        return 0;


    // write data

    write_ioport(ATA_PORT_DATA, buffer, ATA_SECTOR_SIZE);

    return (!ata_wait(ATA_STATUS_BSY));
}
