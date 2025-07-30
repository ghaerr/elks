/**********************************************************************
 * ELKS Generic ATA/IDE functions
 *
 *  Supports dynamic I/O port setup of ATA, XTIDE, XTCF and SOLO/86 controllers:
 *
 *             ATA             XTIDE v1        XTCF                SOLO/86
 * BASE        0x1F0           0x300           0x300               0x40
 * BASE+reg    0x1F0+reg       0x300+reg       0x300+(reg<<1)      0x40+(reg<<1)
 * I/O         16-bit          8-bit           8-bit               16-bit
 *
 *                             XTIDE v2 (high speed mode)
 * BASE+reg                    0x300+swapA0A3(reg)
 *
 * CTRL        BASE+0x200+reg  BASE+0x08+reg   BASE+0x10+(reg<<1)  BASE+0x10+(reg<<1)
 * CTRL+6      BASE+0x206      BASE+0x0E       BASE+0x1C           BASE+0x1C
 * DEVCTL      0x3F6           0x30E           0x31C               0x5C
 *
 *                             XTIDE v2 (high speed mode)
 * CTRL                        BASE+swapA0A3(0x08+reg)
 * CTRL+6                      BASE+swapA0A3(0x0E)
 * DEVCTL                      0x307
 *
 * This code assumes only supports one ATA controller, and sets XTCF operation
 * on 8088/8086 CPUs, unless overriden using xtide= in /bootopts.
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
 * Caveat emptor: This code is based on the ATA specifications,
 * XTIDE Universal BIOS source, and some common sense.
 *
 * Ferry Hendrikx, June 2025
 * Greg Haerr, July 2025 Added XTCF and XTIDE support
 **********************************************************************/

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/genhd.h>
#include <linuxmt/ata.h>
#include <linuxmt/errno.h>
#include <linuxmt/heap.h>
#include <linuxmt/memory.h>
#include <linuxmt/debug.h>
#include <linuxmt/prectimer.h>
#include <arch/io.h>

/* hardware controller access modes, override using xtide= in /bootopts */
#define MODE_ATA        0       /* standard - ATA at ports 0x1F0/0x3F6 */
#define MODE_XTIDEv1    1       /* XTIDE rev1 - ATA at ports 0x300/0x30E, 8-bit I/O */
#define MODE_XTIDEv2    2       /* XTIDE rev2 - XTIDE v1 with registers a0/a3 swapped */
#define MODE_XTCF       3       /* XTCF - XTIDE v1 w/regs << 1 at ports 0x300/0x31C */
#define MODE_SOLO86     4       /* XTCF at ports 0x40/0x5C, 16-bit I/O */
#define MODE_MAX        4
#define AUTO            (-1)    /* use MODE_ATA for PC/AT (286+), MODE_XTCF on PC/XT */

/* hardware I/O port data xfer modes */
#define XFER_16         0       /* ATA/XTIDEv2 16-bit I/O */
#define XFER_8_XTCF     1       /* XTCF set 8-bit feature cmd, then xfer lo then hi */
#define XFER_8_XTIDEv1  2       /* XTIDEv1 8-bit xfer hi at data+8 then lo at data+0 */

/* configurable options */
#define FASTIO          1       /* =1 to use ASM in/out instructions for I/O */
#define EMUL_XTCF       0       /* =1 to force XTCF xfer mode for use with PCem */

static int xfer_mode = AUTO;    /* change this to force a particular I/O xfer method */

/* default base port (change if required) ATA, XTIDEv1, XTIDEv2,  XTCF, SOLO86 */
static unsigned int def_base_ports[5] = { 0x1F0, 0x300,   0x300, 0x300, 0x40 };

/* control register offsets from base ports (should not need changing) */
static unsigned int ctrl_offsets[5] =   { 0x200,  0x08,    0x08,  0x10, 0x10 };

/* table to translate from ATA -> XTIDEV2 register file */
static unsigned int xlate_XTIDEv2[8] = {
    XTIDEV2_DATA,
    XTIDEV2_ERR,
    XTIDEV2_CNT,
    XTIDEV2_LBA_LO,
    XTIDEV2_LBA_MD,
    XTIDEV2_LBA_HI,
    XTIDEV2_SELECT,
    XTIDEV2_STATUS
};

/* wait loop 10ms ticks while busy waiting */
#define WAIT_50MS   (5*HZ/100)      /* 5/100 sec = 50ms while busy */
#define WAIT_10SEC  (10*HZ)         /* 10 sec wait for read/write */

/* end of configurable options */

static unsigned int ata_base_port;
static unsigned int ata_ctrl_port;

/**********************************************************************
 * ATA support functions
 **********************************************************************/

/* convert register number to command block port address */
static int ATPROC BASE(int reg)
{
    if (ata_mode >= MODE_XTCF)          /* XTCF uses SHL 1 register file */
        reg <<= 1;
    else if (ata_mode == MODE_XTIDEv2)  /* XTIDEv2 uses a3/a0 swapped register file */
        reg = xlate_XTIDEv2[reg];
    return ata_base_port + reg;
}

/* input byte from translated register number */
static unsigned char ATPROC INB(int reg)
{
    return inb(BASE(reg));
}

/* output byte to port from translated register number */
/* FIXME: compiler bug if 'unsigned char byte' declared below, bad code in ata_cmd */
static void ATPROC OUTB(unsigned int byte, int reg)
{
    outb(byte, BASE(reg));
}

/* delay 10ms */
static void ATPROC delay_10ms(void)
{
    unsigned long timeout = jiffies + 1 + 1;    /* guarantee at least 10ms interval */

    while (!time_after(jiffies, timeout))
        continue;
}

/**
 * ATA wait 10ms ticks until not busy
 */
static int ATPROC ata_wait(unsigned int ticks)
{
    unsigned long timeout = jiffies + ticks + 1;
    unsigned char status;

    do
    {
        status = INB(ATA_REG_STATUS);

        // are we done?

        if ((status & ATA_STATUS_BSY) == 0)
            return 0;

    } while (!time_after(jiffies, timeout));

    return -ENXIO;
}


/* read from I/O port into far buffer */
static void ATPROC read_ioport(int port, unsigned char __far *buffer, size_t count)
{
    size_t i;

    switch (xfer_mode) {
    case XFER_16:
#if FASTIO
        insw(port, _FP_SEG(buffer), _FP_OFF(buffer), count/2);
#else
        for (i = 0; i < count; i+=2)
        {
            unsigned short word = inw(port);

            *buffer++ = word;
            *buffer++ = word >> 8;
        }
#endif
        break;

    case XFER_8_XTCF:
#if FASTIO
        insb(port, _FP_SEG(buffer), _FP_OFF(buffer), count);
#else
        for (i = 0; i < count; i++)
            *buffer++ = inb(port);
#endif
        break;

    case XFER_8_XTIDEv1:
        for (i = 0; i < count; i+=2)
        {
            *buffer++=  inb(port);      // lo byte first when reading
            *buffer++ = inb(port+8);    // then hi byte from port+8
        }
        break;
    }
}

/* write from far buffer to I/O port */
static void ATPROC write_ioport(int port, unsigned char __far *buffer, size_t count)
{
    size_t i;
    unsigned short word;

    switch (xfer_mode) {
    case XFER_16:
#if FASTIO
        outsw(port, _FP_SEG(buffer), _FP_OFF(buffer), count/2);
#else
        for (i = 0; i < count; i+=2)
        {
            word = *buffer++;
            word |= *buffer++ << 8;
            outw(word, port);
        }
#endif
        break;

    case XFER_8_XTCF:
#if FASTIO
        outsb(port, _FP_SEG(buffer), _FP_OFF(buffer), count);
#else
        for (i = 0; i < count; i++)
            outb(*buffer++, port);
#endif
        break;

    case XFER_8_XTIDEv1:
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
 * ATA try setting 8-bit transfer mode, may be unimplemented by controller
 */
#pragma GCC diagnostic ignored "-Wunused-function"
static int ATPROC ata_set8bitmode(void)
{
    int error;
    unsigned char status;

    // set 8-bit transfer mode

    OUTB(0x01, ATA_REG_FEAT);
    OUTB(ATA_CMD_FEAT, ATA_REG_CMD);


    // wait for drive to be not-busy

    error = ata_wait(WAIT_50MS);
    if (error)
        return error;


    // check for error

    status = INB(ATA_REG_STATUS);

    if (status & ATA_STATUS_ERR)
    {
        printk("cfa: can't set 8-bit xfer\n");
        return -EINVAL;
    }

    printk("cfa: 8-bit xfer on\n");
    return 0;
}

/**
 * ATA select drive
 */
static int ATPROC ata_select(unsigned int drive, unsigned int cmd, unsigned long sector)
{
    unsigned char select = 0xA0 | 0x40 | (drive << 4) | ((sector >> 24) & 0x0F);
    int error;

    // wait for current drive to be non-busy

    error = ata_wait(WAIT_50MS);
    if (error)
        return error;

    // select drive, 400ns wait not required since BSY clear above

    OUTB(select, ATA_REG_SELECT);

    return 0;
}

/**
 * send an ATA command to the drive
 */
static int ATPROC ata_cmd(unsigned int drive, unsigned int cmd, unsigned long sector,
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

    error = ata_wait(cmd == ATA_CMD_READ? WAIT_10SEC: WAIT_50MS);
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
static int ATPROC ata_identify(unsigned int drive, unsigned char __far *buf)
{
    int error;


    // send command

    error = ata_cmd(drive, ATA_CMD_ID, 0, 0);
    if (error)
    {
        printk("cf%c: not found at %x/%x (%d) xtide=%d,%d\n",
            drive+'a', ata_base_port, ata_ctrl_port, error, ata_mode, xfer_mode);
        return error;
    }

    // read data

    read_ioport(BASE(ATA_REG_DATA), buf, ATA_SECTOR_SIZE);

    return 0;
}


/**********************************************************************
 * ATA exported functions
 **********************************************************************/

/**
 * reset the ATA interface
 */
int ATPROC ata_reset(void)
{
    unsigned char byte;

#ifdef CONFIG_ARCH_SOLO86
    ata_mode = MODE_SOLO86;
#endif
#if EMUL_XTCF
    xfer_mode = XFER_8_XTCF;
#endif

    // dynamically set controller access method and I/O port addresses

    if (ata_mode == AUTO || ata_mode > MODE_MAX)
    {
        if (arch_cpu < CPU_80286)       // XTCF is default for 8088/8086 systems
            ata_mode = MODE_XTCF;
        else
            ata_mode = MODE_ATA;        // otherwise standard ATA
    }

    if (xfer_mode == AUTO)
    {
        switch (ata_mode) {
        case MODE_ATA:
        case MODE_SOLO86:
        case MODE_XTIDEv2:
            xfer_mode = XFER_16;
            break;
        case MODE_XTIDEv1:
            xfer_mode = XFER_8_XTIDEv1;
            break;
        case MODE_XTCF:
            xfer_mode = XFER_8_XTCF;
            break;
        }
    }

    // set base port I/O address from emulation mode

    ata_base_port = def_base_ports[ata_mode];

    // set device control register 6 I/O address

    ata_ctrl_port = BASE(6) + ctrl_offsets[ata_mode];
    if (ata_mode == MODE_XTIDEv2)
        ata_ctrl_port ^= 0b1001;        // tricky swap A0 and A3 works only for reg 6

    // controller reset

    byte = INB(ATA_REG_SELECT);
    OUTB(0xA0, ATA_REG_SELECT);
    delay_10ms();
    if (INB(ATA_REG_SELECT) != 0xA0)    // FIXME try probe w/message only for now
    {
        printk("cf: probe fail at %x/%x (%x) xtide=%d\n",
            ata_base_port, ata_ctrl_port, byte, ata_mode);
        OUTB(byte, ATA_REG_SELECT);
        //return -ENODEV;
    }

    // set nIEN and SRST bits

    outb(0x06, ata_ctrl_port);
    delay_10ms();

    // set nIEN bit (and clear SRST bit)

    outb(0x02, ata_ctrl_port);
    delay_10ms();

    // try and turn on 8-bit mode, fallback to 16-bit if controller can't handle it

#if !EMUL_XTCF
    if (xfer_mode == XFER_8_XTCF)
    {
        if (ata_set8bitmode() < 0)
            xfer_mode = XFER_16;
    }
#endif
    return 0;
}


/**
 * initialise an ATA device, return zero if not found
 */
int ATPROC ata_init(int drive, struct drive_infot *drivep)
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
        drivep->cylinders = buffer[ATA_INFO_CYLS];  // FIXME if 0 no cfa: displayed
        drivep->sectors = buffer[ATA_INFO_SPT];
        drivep->heads = buffer[ATA_INFO_HEADS];
        drivep->sector_size = ATA_SECTOR_SIZE;
        drivep->fdtype = HARDDISK;
        show_drive_info(drivep, "cf", drive, 1, " ");

        // now display extra info: ATA LBA sector total, version and sector size

        total = (sector_t)buffer[ATA_INFO_SECT_HI] << 16 | buffer[ATA_INFO_SECT_LO];
        printk("%luK VER %x/%d xtide=%d,%d", total >> 1, buffer[ATA_INFO_VER_MAJ],
            buffer[ATA_INFO_SECT_SZ], ata_mode, xfer_mode);

        // Sanity check
        if ((buffer[ATA_INFO_CYLS] == 0 || buffer[ATA_INFO_CYLS] > 0x7F00) ||
            (buffer[ATA_INFO_HEADS] == 0 || buffer[ATA_INFO_HEADS] > 16) ||
            (buffer[ATA_INFO_SPT] == 0 || buffer[ATA_INFO_SPT] > 63))
        {
            printk(" (unsupported format)");
            drivep->cylinders = 0;
        }
        if (! (buffer[ATA_INFO_CAPS] & ATA_CAPS_LBA))   // ATA LBA support?
        {
            printk(" (missing LBA)");
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
int ATPROC ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
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

    return 0;
}


/**
 * write to an ATA device
 *
 * drive : physical drive number (0 or 1)
 * sector: sector number
 * buf/seg: I/O buffer address
 */
int ATPROC ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg)
{
    unsigned char __far *buffer;
    int error;
    unsigned char status;


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
        buffer = _MK_FP(DMASEG, 0);
        write_ioport(BASE(ATA_REG_DATA), buffer, ATA_SECTOR_SIZE);
    }
    else
#endif
    {
        buffer = _MK_FP(seg, buf);
        write_ioport(BASE(ATA_REG_DATA), buffer, ATA_SECTOR_SIZE);
    }

    error = ata_wait(WAIT_10SEC);
    if (error)
        return error;

    // check for write error

    status = INB(ATA_REG_STATUS);

    if (status & (ATA_STATUS_ERR|ATA_STATUS_DFE))
        return -EIO;

    return 0;
}
