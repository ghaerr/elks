/**********************************************************************
 * ELKS Generic ATA/IDE functions
 *
 *  Supports dynamic I/O port setup of ATA, XTIDE, XTCF and SOLO/86 controllers:
 *
 *             ATA             XTIDE           XTCF                SOLO/86
 * BASE        0x1F0           0x300           0x300               0x40
 * BASE+reg    0x1F0+reg       0x300+reg       0x300+(reg<<1)      0x40+(reg<<1)
 *
 * CTRL        BASE+0x200+reg  BASE+0x08+reg   BASE+0x10+(reg<<1)  BASE+0x10+(reg<<1)
 * CTRL+6      0x3F6           0x30E           0x31C               0x5C
 * DEVCTRL     BASE+0x206      BASE+0x0E       BASE+0x1C           BASE+0x1C
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
 * Greg Haerr, July 2025 Added XTCF support
 **********************************************************************/

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/genhd.h>
#include <linuxmt/errno.h>
#include <linuxmt/heap.h>
#include <linuxmt/debug.h>
#include <arch/io.h>
#include <arch/ata.h>

/* wait loop counts while busy waiting (FIXME: use jiffies for accuracy) */
#define SHORT_WAIT  500
#define LONG_WAIT   5000        /* 14s wait required for some writes */

/* controller emulation modes */
#define MODE_ATA    0
#define MODE_XTIDE  1
#define MODE_XTCF   2
#define MODE_SOLO86 3

/* default base ports for ATA, XTIDE, XTCF and SOLO86 */
static unsigned int def_base_ports[4] = { 0x1F0, 0x300, 0x300, 0x40 };

/* control register offsets from base ports */
static unsigned int ctrl_offsets[4] =   { 0x200, 0x08, 0x10, 0x10 };

static int mode = MODE_ATA;
static unsigned int ata_base_port;
static unsigned int ata_ctrl_port;
static char use_8bitmode;

/* convert register number to base port address */
#define BASE(reg)   ((mode < MODE_XTCF)? (ata_base_port+reg): (ata_base_port+((reg)<<1)))

/**********************************************************************
 * ATA support functions
 **********************************************************************/

/* input byte from translated register number */
static unsigned char INB(int reg)
{
    return inb(BASE(reg));
}

/* output byte to port from translated register number */
/* FIXME: compiler bug if 'unsigned char byte' declared below, bad code in ata_cmd */
static void OUTB(unsigned int byte, int reg)
{
    outb(byte, BASE(reg));
}

/**
 * ATA delay
 */
static void ata_delay(void)
{
    int i = 1000;

    while (i--) asm("nop");
}

/**
 * ATA delay 400ns
 */
static void ata_delay_400(void)
{
    int i;

    for (i = 0; i < 15; i++)
        INB(ATA_REG_STATUS);
}

/**
 * ATA wait until not busy
 */
static int ata_wait(int loops)
{
    int i;
    unsigned char status;

    for (i = 0; i < loops; i++)
    {
        status = INB(ATA_REG_STATUS);

        // are we done?

        if ((status & ATA_STATUS_BSY) == 0)
            return 0;

        ata_delay();
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
            buffer[i] = inb(port);
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
            outb(buffer[i], port);
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
 * ATA select drive
 */
static int ata_select(unsigned int drive, unsigned int cmd, unsigned long sector)
{
    unsigned char select = 0xA0 | 0x40 | (drive << 4) | ((sector >> 24) & 0x0F);
    int error;

    // wait for drive to be non-busy

    error = ata_wait(SHORT_WAIT);
    if (error)
        return error;

    // send

    OUTB(select, ATA_REG_DRVH);

    ata_delay_400();

    // wait for drive to be non-busy

    return ata_wait(SHORT_WAIT);
}


/**
 * ATA set 8-bit transfer mode if possible
 */
static int ata_set8bitmode(void)
{
    int error;
    unsigned char status;

    // set 8-bit transfer mode

    OUTB(0x01, ATA_REG_FEAT);
    OUTB(ATA_CMD_FEAT, ATA_REG_CMD);


    // wait for drive to be not-busy

    error = ata_wait(SHORT_WAIT);
    if (error)
        return error;


    // check for error

    status = INB(ATA_REG_STATUS);

    if (status & ATA_STATUS_ERR)
    {
        printk("cf: can't set 8-bit transfer mode (error %02xh)\n", INB(ATA_REG_ERR));
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
    int error;
    unsigned char status;


    // send command

    error = ata_select(drive, cmd, sector);
    if (error)
        return error;

    OUTB(0x00, ATA_REG_FEAT);
    OUTB(count, ATA_REG_CNT);
    OUTB((unsigned char) (sector),       ATA_REG_LBA_LO);
    OUTB((unsigned char) (sector >> 8),  ATA_REG_LBA_MD);
    OUTB((unsigned char) (sector >> 16), ATA_REG_LBA_HI); // FIXME OUTB compiler bug here
    OUTB(cmd, ATA_REG_CMD);


    // wait for drive to be not-busy

    error = ata_wait(cmd == ATA_CMD_READ? LONG_WAIT: SHORT_WAIT);
    if (error)
        return error;


    // check for error

    status = INB(ATA_REG_STATUS);

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
    {
        printk("cf%d: ATA port %x/%x, probe failed (%d)\n",
            drive, ata_base_port, ata_ctrl_port, error);
        return error;
    }

    // read data

    read_ioport(BASE(ATA_REG_DATA), buf, ATA_SECTOR_SIZE);

    return ata_wait(SHORT_WAIT);
}


/**********************************************************************
 * ATA exported functions
 **********************************************************************/

/**
 * reset the ATA interface
 */
void ata_reset(void)
{
    // dynamically set I/O port addresses

#ifdef CONFIG_ARCH_SOLO86
    mode = MODE_SOLO86
#else
    if (arch_cpu < 6)       // prior to 80286 IBM PC/AT
        mode = MODE_XTCF;
#endif

    // set base port I/O address from emulation mode
    ata_base_port = def_base_ports[mode];

    // set device control register 6 I/O address
    ata_ctrl_port = BASE(6) + ctrl_offsets[mode];

    // controller reset

    OUTB(0xA0, ATA_REG_DRVH);

    ata_delay();

    // set nIEN and SRST bits
    outb(0x06, ata_ctrl_port);

    ata_delay();

    // set nIEN bit (and clear SRST bit)
    outb(0x02, ata_ctrl_port);

    ata_delay();


    // controller 8-bit transfer is requested for 8086/80186 systems:
    // try and turn on 8-bit mode

    use_8bitmode = 0;

    if (arch_cpu < 6)
        use_8bitmode = (ata_set8bitmode() == 0);
}


/**
 * initialise an ATA device
 */
int ata_init(int drive, struct drive_infot *drivep)
{
    unsigned short *buffer;
    sector_t total;
    int ret = 0;


    // allocate buffer

    buffer = (unsigned short *) heap_alloc(ATA_SECTOR_SIZE, HEAP_TAG_DRVR|HEAP_TAG_CLEAR);

    if (!buffer)
        return 0;

    // identify drive

    if (ata_identify(drive, (unsigned char __far *) buffer) == 0)
    {
        // ATA LBA sector total (MSB << 16, LSB)
        total = (sector_t)buffer[ATA_INFO_SECT_HI] << 16 | buffer[ATA_INFO_SECT_LO];

        printk("cf%d: ATA port %x/%x, %luK CHS %2u,%2u,%2u ",
            drive, ata_base_port, ata_ctrl_port, total >> 1,
            buffer[ATA_INFO_CYLS], buffer[ATA_INFO_HEADS],
            buffer[ATA_INFO_SPT]);

        // ATA version
        if ((buffer[ATA_INFO_VER_MAJ] != 0) && (buffer[ATA_INFO_VER_MAJ] != 0xFFFF))
        {
            // dump out version word (don't bother decoding it)

            printk("VER %x ", buffer[ATA_INFO_VER_MAJ]);
        }

        // Sanity check
        if ((buffer[ATA_INFO_CYLS] == 0 || buffer[ATA_INFO_CYLS] > 0x7F00) ||
            (buffer[ATA_INFO_HEADS] == 0 || buffer[ATA_INFO_HEADS] > 16) ||
            (buffer[ATA_INFO_SPT] == 0 || buffer[ATA_INFO_SPT] > 63) ||
            (buffer[ATA_INFO_SECT_SZ] != ATA_SECTOR_SIZE))
        {
            printk("invalid CF");
            ret = 0;
        }
        else if (! (buffer[ATA_INFO_CAPS] & ATA_CAPS_LBA))      // ATA LBA support?
        {
            printk("no LBA");
            ret = 0;
        }
        else
            ret = 1;
        drivep->cylinders = buffer[ATA_INFO_CYLS];
        drivep->sectors = buffer[ATA_INFO_SPT];
        drivep->heads = buffer[ATA_INFO_HEADS];
        drivep->sector_size = ATA_SECTOR_SIZE;
        drivep->fdtype = -1;

        printk("\n");
    }

    heap_free(buffer);

    return ret;
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
    unsigned char __far *buffer;
    int error;


    // send command

    error = ata_cmd(drive, ATA_CMD_READ, sector, 1);
    if (error)
        return error;


    // read data

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#ifdef CONFIG_FS_XMS
    int use_xms = seg >> 16;
    if (use_xms)
    {
        buffer = _MK_FP(DMASEG, 0);
        read_ioport(BASE(ATA_REG_DATA), buffer, ATA_SECTOR_SIZE);
        xms_fmemcpyw(buf, seg, 0, DMASEG, ATA_SECTOR_SIZE / 2);
    }
    else
#endif
    {
        buffer = _MK_FP(seg, buf);
        read_ioport(BASE(ATA_REG_DATA), buffer, ATA_SECTOR_SIZE);
    }

    return ata_wait(SHORT_WAIT);
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
    unsigned char __far *buffer;
    int error;


    // send command

    error = ata_cmd(drive, ATA_CMD_WRITE, sector, 1);
    if (error)
        return error;


    // write data

#ifdef CONFIG_FS_XMS
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
    int use_xms = seg >> 16;
    if (use_xms)
    {
        xms_fmemcpyw(0, DMASEG, buf, seg, ATA_SECTOR_SIZE / 2);
        buffer = _MK_FP(seg, buf);
        write_ioport(BASE(ATA_REG_DATA), buffer, ATA_SECTOR_SIZE);
    }
    else
#endif
    {
        buffer = _MK_FP(seg, buf);
        write_ioport(BASE(ATA_REG_DATA), buffer, ATA_SECTOR_SIZE);
    }


    return ata_wait(LONG_WAIT);
}
