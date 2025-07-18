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
 * This code assumes there is only one ATA controller, and normally tries to
 * set 8-bit transfer mode on 8088/8086/NEC V20 CPUs.
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

/* hardware controller access modes */
#define MODE_ATA    0           /* standard - ATA at ports 0x1F0/0x3F6 */
#define MODE_XTIDE  1           /* XTIDE - ATA at ports 0x300/0x30E */
#define MODE_XTCF   2           /* XTCF - non-standard ATA at ports 0x300/0x31C */
#define MODE_SOLO86 3           /* XTCF at ports 0x40/0x5C, 16-bit I/O */
#define AUTO        (-1)        /* use MODE_ATA for PC/AT (286+), MODE_XTCF on PC/XT */

/* hardware I/O port xfer modes */
#define XFER_16BIT  0           /* standard 16-bit ATA I/O */
#define XFER_8XTCF  1           /* XTCF set 8-bit feature cmd, then xfer lo then hi */
#define XFER_8XTIDE 2           /* XTIDE 8-bit xfer hi at data+8 then lo at data+0 */

/* configurable options - may have to be changed and kernel recompiled*/

static int mode = AUTO;         /* change this to force a particular controller */
static int xfer_mode = AUTO;    /* change this to force a particular I/O xfer method */

/* default base port (change if required)   ATA, XTIDE,  XTCF, SOLO86 */
static unsigned int def_base_ports[4] = { 0x1F0, 0x300, 0x300, 0x40 };

/* control register offsets from base ports (should not need changing) */
static unsigned int ctrl_offsets[4] =   { 0x200, 0x08, 0x10, 0x10 };

/* wait loop counts while busy waiting (FIXME: use jiffies for accuracy) */
#define SHORT_WAIT  500
#define LONG_WAIT   5000        /* FIXME: 14s wait required for some writes */

/* end of configurable options */

static unsigned int ata_base_port;
static unsigned int ata_ctrl_port;

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
    unsigned short word;

    switch (xfer_mode) {
    case XFER_16BIT:
        for (i = 0; i < count; i+=2)
        {
            word = inw(port);

            *buffer++ = word;
            *buffer++ = word >> 8;
        }
        break;

    case XFER_8XTCF:
        for (i = 0; i < count; i++)
            *buffer++ = inb(port);
        break;

    case XFER_8XTIDE:
        for (i = 0; i < count; i+=2)
        {
            *buffer++=  inb(port);      // lo byte first when reading
            *buffer++ = inb(port+8);    // then hi byte from port+8
        }
        break;
    }
}

/* write from far buffer to I/O port */
static void write_ioport(int port, unsigned char __far *buffer, size_t count)
{
    size_t i;
    unsigned short word;

    switch (xfer_mode) {
    case XFER_16BIT:
        for (i = 0; i < count; i+=2)
        {
            word = *buffer++;
            word |= *buffer++ << 8;
            outw(word, port);
        }
        break;

    case XFER_8XTCF:
        for (i = 0; i < count; i++)
            outb(*buffer++, port);
        break;

    case XFER_8XTIDE:
        for (i = 0; i < count; i+=2)
        {
            word = *buffer++;           // save low byte
            outb(*buffer++, port+8);    // hi byte first to port+8
            outb(word, port);           // then lo byte to port+0
        }
        break;
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
        printk("cf%c: not found at %x/%x (%d)\n",
            drive+'a', ata_base_port, ata_ctrl_port, error);
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
    mode = MODE_SOLO86;
    xfer_mode = XFER_16BIT;
#else
    if (mode == AUTO)
    {
        mode = MODE_ATA;
        if (arch_cpu < 6)           // prior to 80286 IBM PC/AT
            mode = MODE_XTCF;
    }
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

    // 8-bit transfer is default for 8086/80186 systems

    if (xfer_mode == AUTO)
    {
        if (arch_cpu <= CPU_80186)
        {
            if (mode == MODE_XTCF)
                xfer_mode = XFER_8XTCF;
            else
                xfer_mode = XFER_8XTIDE;
        }
        else
            xfer_mode = XFER_16BIT;
    }

    // try and turn on 8-bit mode, fallback to 16-bit if controller can't handle it

    if (xfer_mode == XFER_8XTCF)
    {
        if (ata_set8bitmode() < 0)
            xfer_mode = XFER_16BIT;
    }
}


/**
 * initialise an ATA device, return zero if not found
 */
int ata_init(int drive, struct drive_infot *drivep)
{
    unsigned short *buffer;
    sector_t total;

    drivep->cylinders = 0;      // invalidate device

    buffer = (unsigned short *) heap_alloc(ATA_SECTOR_SIZE, HEAP_TAG_DRVR|HEAP_TAG_CLEAR);
    if (!buffer)
        return 0;

    // identify drive

    if (ata_identify(drive, (unsigned char __far *) buffer) == 0)
    {
        drivep->cylinders = buffer[ATA_INFO_CYLS];
        drivep->sectors = buffer[ATA_INFO_SPT];
        drivep->heads = buffer[ATA_INFO_HEADS];
        drivep->sector_size = ATA_SECTOR_SIZE;
        drivep->fdtype = -1;
        show_drive_info(drivep, "cf", drive, 1, " ");

        // now display extra info: ATA LBA sector total, version and sector size

        total = (sector_t)buffer[ATA_INFO_SECT_HI] << 16 | buffer[ATA_INFO_SECT_LO];
        printk("%luK VER %x/%d ", total >> 1, buffer[ATA_INFO_VER_MAJ],
            buffer[ATA_INFO_SECT_SZ]);

        // Sanity check
        if ((buffer[ATA_INFO_CYLS] == 0 || buffer[ATA_INFO_CYLS] > 0x7F00) ||
            (buffer[ATA_INFO_HEADS] == 0 || buffer[ATA_INFO_HEADS] > 16) ||
            (buffer[ATA_INFO_SPT] == 0 || buffer[ATA_INFO_SPT] > 63))
        {
            printk("(unsupported format) ");
            drivep->cylinders = 0;
        }
        if (! (buffer[ATA_INFO_CAPS] & ATA_CAPS_LBA))   // ATA LBA support?
        {
            printk("(missing LBA)");
            drivep->cylinders = 0;
        }
        printk("\n");
    }

    heap_free(buffer);

    return drivep->cylinders;
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
