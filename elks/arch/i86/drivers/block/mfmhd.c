/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * XT MFM hard disk driver.
 *
 * This driver talks directly to XT Winchester controllers using the 4-port
 * Xebec-style interface: command/status bytes use programmed I/O and sector
 * data uses the PC/XT 8237 DMA controller's fixed hard-disk channel 3.
 *
 * These are ST-506/412 controllers, not IDE/ATA, ESDI, or generic 26-sector
 * RLL controllers.  The ST11R is supported only through the documented
 * Seagate command profile.  The primary target is the early Western Digital
 * "Quebec" style WD1002 command-block family:
 *
 *   - Western Digital WD1002A-WX1, including Super BIOS, G, and H tables
 *   - WD1002-compatible 8-bit ISA MFM boards exposing the same 0x320/0x324
 *     4-port command interface and 17-sector ST-506/412 format
 *   - Seagate ST11M/ST11R and FileCard-class controllers through the ST11
 *     geometry command path
 *   - Early Xebec/IBM XT fixed-disk command-block adapters may share enough
 *     protocol to work, but should be verified before enabling writes
 *
 * WD1002A-WX1 documentation allows up to 1024 cylinders and 16 heads.  The
 * normal WD MFM format is 17 sectors/track; fixed CHS overrides, BIOS switch
 * tables, sector-0 WD geometry, and FAT BPB probing let the same lean driver
 * handle smaller and larger supported drives without carrying a full BIOS
 * translation layer.
 *
 * The driver is fully synchronous (not ASYNC_IO): one controller command is
 * in flight at a time and completion is polled with IRQ delivery masked, so
 * the static command scratch blocks below are never shared between two
 * commands and no sector- or command-sized array ever lands on the small
 * ELKS kernel stack.
 *
 * One controller with up to two ST-506/412 units is supported, as /dev/mfma
 * and /dev/mfmb.
 *
 * The command construction and controller phase sequencing are adapted from
 * Linux drivers/block/xd.c, originally written by Pat Mackinlay, with work
 * credited there to Risto Kankkunen, Todd Fries, Andrzej Krzysztofowicz,
 * and other contributors.
 *
 * ELKS adaptation: G Keet <gatekeeper@xt-emporium.com>, 2026.
 */

#include <linuxmt/config.h>
#include <linuxmt/major.h>
#include <linuxmt/devnum.h>
#include <linuxmt/genhd.h>
#include <linuxmt/fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/heap.h>
#include <linuxmt/string.h>
#include <linuxmt/mm.h>
#include <linuxmt/debug.h>
#include <linuxmt/init.h>
#include <arch/io.h>
#include <arch/dma.h>
#include <arch/irq.h>
#include <arch/ports.h>
#include <arch/segment.h>
#include <arch/system.h>
#include <arch/divmod.h>
#include <arch/mfmhd.h>

#define MAJOR_NR        MFMHD_MAJOR
#include "blk.h"

/* WD1002A-WX1 register offsets from the selected I/O base. */
#define MFM_HD_DATA             0
#define MFM_HD_STATUS           1
#define MFM_HD_RESET            1
#define MFM_HD_SELECT           2
#define MFM_HD_JUMPER           2
#define MFM_HD_CONTROL          3

/* Hardware status register bits, mirrored in arch/mfmhd.h for user tools. */
#define MFM_STAT_READY          MFMHD_STAT_READY
#define MFM_STAT_INPUT          MFMHD_STAT_INPUT
#define MFM_STAT_COMMAND        MFMHD_STAT_COMMAND
#define MFM_STAT_SELECT         MFMHD_STAT_SELECT
#define MFM_STAT_REQUEST        MFMHD_STAT_REQUEST
#define MFM_STAT_INTERRUPT      MFMHD_STAT_INTERRUPT
#define MFM_STAT_PHASE_MASK     (MFM_STAT_COMMAND | MFM_STAT_INPUT)

#define MFM_PHASE_DATA_OUT      0
#define MFM_PHASE_DATA_IN       MFM_STAT_INPUT
#define MFM_PHASE_CMD_OUT       MFM_STAT_COMMAND
#define MFM_PHASE_STATUS_IN     (MFM_STAT_COMMAND | MFM_STAT_INPUT)

/* Port base+3 control bits; sector DMA is enabled without controller IRQs. */
#define MFM_CTL_DRQEN           MFMHD_CTL_DRQEN
#define MFM_CTL_IRQEN           MFMHD_CTL_IRQEN
#define MFM_CTL_PIO_POLLED      MFMHD_CTL_PIO_POLLED
#define MFM_CTL_VALID_MASK      (MFM_CTL_DRQEN | MFM_CTL_IRQEN)

/* Command-status byte bits. */
#define MFM_CSB_ERROR           0x02
#define MFM_CSB_LUN             0x20

/* WD1002/Xebec six-byte command block operations. */
#define MFM_CMD_TEST_DRIVE_READY        MFMHD_CMD_TEST_DRIVE_READY
#define MFM_CMD_RECALIBRATE             MFMHD_CMD_RECALIBRATE
#define MFM_CMD_SENSE                   MFMHD_CMD_READ_STATUS
#define MFM_CMD_SEEK                    MFMHD_CMD_SEEK
#define MFM_CMD_READ_SECTORS            MFMHD_CMD_READ_SECTORS
#define MFM_CMD_WRITE_SECTORS           MFMHD_CMD_WRITE_SECTORS
#define MFM_CMD_INIT_DRIVE_PARAMETERS   MFMHD_CMD_INIT_DRIVE_PARAMETERS
#define MFM_CMD_ST11_GET_GEOMETRY       MFMHD_CMD_ST11_GET_GEOMETRY

/* Command block byte 5: R1/R2/0/0/0/SP2/SP1/SP0. */
#define MFM_CCB_R1_NO_RETRY         MFMHD_CCB_R1_NO_RETRY
#define MFM_CCB_R2_IMMEDIATE_ECC    MFMHD_CCB_R2_IMMEDIATE_ECC
#define MFM_CCB_STEP_MASK           MFMHD_CCB_STEP_MASK
#define MFM_CCB_RESERVED_MASK       MFMHD_CCB_RESERVED_MASK

/* Choose the WD1002A-WX1 BIOS switch table used for jumper geometry. */
#define MFMHD_BIOS_SUPER        0
#define MFMHD_BIOS_G            1
#define MFMHD_BIOS_H            2

#define MFMHD_CTRL_WD1002       0
#define MFMHD_CTRL_SEAGATE      1

/*
 * Port, IRQ and flags come from mfm= in /bootopts, defaulting to the MFM_
 * settings in ports.h.  The rarely-used tunables below live here rather than
 * in .config; edit and rebuild for non-default hardware.  The one genuinely
 * board-level choice remains a menuconfig option: CONFIG_MFMHD_SEAGATE.
 */
#define mfm_irq                 (mfm_conf.irq)
#define mfm_port                (mfm_conf.port)
#define mfm_flags               (mfm_conf.flags)

/* mfm= flags field, each explained at the variable it sets. */
#define MFMF_SLOW               1       /* conservative controller deadlines */
#define MFMF_PIO                2       /* sector payloads over PIO, not DMA */
#define MFMF_TRACE              4       /* request tracing to the console */
#define MFMF_IRQ                8       /* interrupt-driven command completion */
#define MFMF_EXTWRITE           16      /* 8237 Extended Write strobe */

/* Controller command profile: WD1002A-WX1 or Seagate ST11M/R/FileCard. */
#ifdef CONFIG_MFMHD_SEAGATE
#define MFMHD_CONTROLLER        MFMHD_CTRL_SEAGATE
#else
#define MFMHD_CONTROLLER        MFMHD_CTRL_WD1002
#endif

/*
 * Controller units to probe (1 or 2).  Must be 1 for FileCard 20 split-unit
 * geometry, which presents both physical units as a single /dev/mfma.
 */
#ifndef MFMHD_DRIVES
#define MFMHD_DRIVES            2
#endif

/* WD1002A-WX1 BIOS ROM switch table revision used for jumper geometry. */
#ifndef MFMHD_WD1002_BIOS
#define MFMHD_WD1002_BIOS       MFMHD_BIOS_SUPER
#endif

/* WD1002/Xebec command-control byte step-rate code (byte 5 low bits). */
#ifndef MFMHD_STEP_CODE
#define MFMHD_STEP_CODE         MFMHD_STEP_3MS_PREFERRED
#endif

/* Set R1 (disable controller retries) / R2 (immediate ECC) in byte 5. */
#ifndef MFMHD_NO_RETRY
#define MFMHD_NO_RETRY          0
#endif
#ifndef MFMHD_ECC_IMMEDIATE
#define MFMHD_ECC_IMMEDIATE     0
#endif

/*
 * Fixed per-drive CHS geometry overrides for drives whose controller table
 * or geometry command does not match the disk.  Zero keeps probed geometry;
 * cylinders and heads set with sectors zero defaults to 17 sectors.
 */
#ifndef MFMHD_DRIVE0_CYLINDERS
#define MFMHD_DRIVE0_CYLINDERS  0
#endif
#ifndef MFMHD_DRIVE0_HEADS
#define MFMHD_DRIVE0_HEADS      0
#endif
#ifndef MFMHD_DRIVE0_SECTORS
#define MFMHD_DRIVE0_SECTORS    0
#endif
#ifndef MFMHD_DRIVE1_CYLINDERS
#define MFMHD_DRIVE1_CYLINDERS  0
#endif
#ifndef MFMHD_DRIVE1_HEADS
#define MFMHD_DRIVE1_HEADS      0
#endif
#ifndef MFMHD_DRIVE1_SECTORS
#define MFMHD_DRIVE1_SECTORS    0
#endif

/* Maintenance/diagnostic MFMHDIOC_* ioctls; 0 keeps the kernel smallest. */
/* Allow MFMHDIOC_SETOPTS to write the port control register directly. */

/* Command handshake tracing for bring-up. */

/* Raw jumper port polarity.  Most ISA jumper inputs read 1 when open. */
#ifndef MFMHD_JUMPER_OPEN_IS_ONE
#define MFMHD_JUMPER_OPEN_IS_ONE    1
#endif

#define MFM_COMMAND_BYTES       6
#define MFM_SETPARAM_PARAMETER_BYTES 8
#define MFM_SENSE_BYTES         4
#define MFM_SECTOR_BYTES        512
#define MFM_BOUNCE_SECTORS      1
/*
 * Normal requests use the first 512 bytes.  Maintenance READ LONG and
 * WRITE LONG commands transfer four additional ECC bytes.  Reserving those
 * bytes here avoids ever placing a sector-sized object on the tiny ELKS
 * kernel stack.
 */
#define MFM_BOUNCE_BYTES       MFM_SECTOR_BYTES

/*
 * An original PC/XT 8237 channel cannot cross a physical 64 KiB boundary.
 * ELKS reserves DMASEGSZ bytes at the fixed physical segment SEG_DMASEG,
 * which is below that boundary on IBM PC builds.  The CPU addresses the same
 * memory through DMASEG:0; DMASEG is either the real-mode segment or the
 * matching protected-mode selector.  DMASEGSZ is 1024 bytes for this driver,
 * so normal requests transfer at most two 512-byte sectors at a time.  The
 * 516-byte READ/WRITE LONG maintenance payload also fits.
 */
#if DMASEGSZ < MFM_BOUNCE_BYTES
#error "XT MFM driver requires more low DMA memory than DMASEGSZ provides"
#endif
#define MFM_DMA_CHANNEL        3
#define MFM_DMA_ADDRESS_PORT   DMA_ADDR_3
#define MFM_DMA_COUNT_PORT     DMA_CNT_3
#define MFM_DMA_PAGE_PORT      DMA_PAGE_3
#define MFM_DMA_BYTES          DMASEGSZ
#define MFM_DMA_SECTORS        (MFM_DMA_BYTES / MFM_SECTOR_BYTES)

/* IBM 8086 words are little-endian; byte access avoids costly shifts by 8. */
union mfm_dma_word {
    unsigned int word;
    unsigned char byte[2];
};

/*
 * sector_t is ELKS's existing two-word block-address ABI.  Keep its word
 * layout explicit at disk-format and block-layer boundaries so the compiler
 * never needs a 32-bit shift or multiply.  This layout is little-endian, as
 * required by the 8086 target and by the FAT fields read below.
 */
union mfm_sector_words {
    sector_t value;
    struct {
        unsigned int low;
        unsigned int high;
    } word;
};

/* One controller with two ST-506/412 units: /dev/mfma and /dev/mfmb. */
#define MFM_MAX_DRIVES          2
#define MFM_MINOR_SHIFT         3
#define MFM_MAX_PARTITIONS      (1 << MFM_MINOR_SHIFT)

#if MFMHD_DRIVES < 1 || MFMHD_DRIVES > MFM_MAX_DRIVES
#error MFMHD_DRIVES must be 1 or 2
#endif

#define MFM_WD1002_MAX_CYLINDERS    1024
#define MFM_WD1002_MAX_HEADS        16
#define MFM_WD1002_SECTORS          17
#define MFM_WD1002_MAX_BURST        1       /* WD1002 does not reliably wrap */
#define MFM_ST11_MAX_BURST          0x40
/*
 * FileCard 20 is a 612/4/17 physical ST-506 disk.  Some FileCard BIOSes
 * present that as one DOS disk while the controller command interface exposes
 * it as two 306/4/17 units.  Keep /dev/mfma as the full disk and split command
 * CHS at the half-disk boundary.
 */
#define MFM_FILECARD20_UNIT_CYLINDERS   306
#define MFM_FILECARD20_UNITS        2
#define MFM_FILECARD20_CYLINDERS    \
    (MFM_FILECARD20_UNIT_CYLINDERS * MFM_FILECARD20_UNITS)
#define MFM_FILECARD20_HEADS        4
#define MFM_FILECARD20_SECTORS      17
#define MFM_FILECARD20_UNIT_SECTORS \
    (MFM_FILECARD20_UNIT_CYLINDERS * MFM_FILECARD20_HEADS * \
     MFM_FILECARD20_SECTORS)

#define MFM_RETRIES             4
#define MFM_MBR_READ_TRIES      2
/*
 * Ceiling on the whole boot-time probe.  Every individual wait is already
 * bounded, but their product is not: a controller that answers slowly can
 * multiply retries and recalibrations into minutes of apparent hang before
 * the root filesystem is even mounted.  Runtime I/O keeps its full retry
 * budget; only the probe is cut short.
 */
#define MFM_PROBE_TICKS         (HZ * 30)
/*
 * Spin backstop for every wait in this driver.
 *
 * ll_rw_blk's unplug_device() calls the request function with interrupts
 * disabled, and jiffies only advances from the timer interrupt.  A wait
 * that trusts jiffies alone therefore never expires when it is reached
 * that way: the machine stops dead, keyboard included, instead of the
 * command failing.  Count iterations as well, so every loop terminates
 * whatever the interrupt state.  The counter is deliberately generous --
 * it is a safety net, not the timing source.
 */
#define MFM_SPIN_ROUNDS         48      /* x 65536 status reads */
#define MFM_PHASE_TICKS         (HZ * 2)
#define MFM_DISK_TICKS          (HZ * 12)
#define MFM_SELECT_TICKS        (HZ * 2)
#define MFM_SLOW_PHASE_TICKS    (HZ * 4)
#define MFM_SLOW_DISK_TICKS     (HZ * 24)
#define MFM_SLOW_SELECT_TICKS   (HZ * 4)
/*
 * Formatting is an optional maintenance operation.  A 10-minute ceiling is
 * long enough for XT media while keeping the timeout constant within a
 * 16-bit unsigned word.  The jiffies interface itself is the existing ELKS
 * two-word clock ABI; no wide multiply is emitted here.
 */
#define MFM_SINGLE_RECOVER_OK   64
#define MFM_DEFAULT_ECC_LENGTH  11

#define MFMHD_GEO_NONE          0xffff

#if MFMHD_CONTROLLER == MFMHD_CTRL_SEAGATE
#define MFMHD_MAX_BURST         MFM_ST11_MAX_BURST
#define mfmhd_controller_is_seagate()   1
#else
#define MFMHD_MAX_BURST         MFM_WD1002_MAX_BURST
#define mfmhd_controller_is_seagate()   0
#endif

#define STATUS(port)            inb_p((port) + MFM_HD_STATUS)
#define DATA(port)              inb_p((port) + MFM_HD_DATA)
#define DATA_OUT(port, val)     outb_p((val), (port) + MFM_HD_DATA)

#define MFM_LE16(b, o) \
    ((unsigned int)(unsigned char)(b)[(o)] | \
    ((unsigned int)(unsigned char)(b)[(o) + 1] << 8))

struct mfm_wd1002_geom {
    unsigned short cylinders;
    unsigned char heads;
    unsigned char sectors;
    unsigned short wp_cylinder;
    unsigned short rwc_cylinder;
};

struct mfm_drive_info {
    unsigned int cylinders;
    unsigned int sectors;
    unsigned int heads;
    unsigned short wp_cylinder;
    unsigned short rwc_cylinder;
    unsigned char source;
    unsigned char force_single;
    unsigned char single_ok_streak;
    unsigned char cmd_control;
    unsigned char last_csb;
    unsigned char last_sense[MFM_SENSE_BYTES];
    unsigned int retry_count;
    unsigned int cmd_error_count;
    unsigned int io_error_count;
};

#define MFM_GEO_SRC_NONE        0
#define MFM_GEO_SRC_USER        1
#define MFM_GEO_SRC_WD1002      2
#define MFM_GEO_SRC_BPB         3
#define MFM_GEO_SRC_SEAGATE     4
#define MFM_GEO_SRC_FILECARD20  5

#if MFMHD_WD1002_BIOS == MFMHD_BIOS_SUPER
/* WD1002A-WX1 BIOS revision "SUPER BIOS" drive type table. */
static const struct mfm_wd1002_geom mfmhd_wd1002_super_table[4] = {
    { 612, 4, MFM_WD1002_SECTORS, 450, 450 },
    { 306, 4, MFM_WD1002_SECTORS, 0,   153 },
    { 615, 2, MFM_WD1002_SECTORS, 450, 450 },
    { 615, 4, MFM_WD1002_SECTORS, 450, 450 }
};
#elif MFMHD_WD1002_BIOS == MFMHD_BIOS_G
/* WD1002A-WX1 BIOS revision "G" drive type table. */
static const struct mfm_wd1002_geom mfmhd_wd1002_g_table[4] = {
    { 612, 4, MFM_WD1002_SECTORS, MFMHD_GEO_NONE, MFMHD_GEO_NONE },
    { 612, 2, MFM_WD1002_SECTORS, 128,            128 },
    { 612, 4, MFM_WD1002_SECTORS, 128,            MFMHD_GEO_NONE },
    { 306, 4, MFM_WD1002_SECTORS, 0,              MFMHD_GEO_NONE }
};
#elif MFMHD_WD1002_BIOS == MFMHD_BIOS_H
/* WD1002A-WX1 BIOS revision "H" drive type table. */
static const struct mfm_wd1002_geom mfmhd_wd1002_h_table[8] = {
    { 977,  5, MFM_WD1002_SECTORS, MFMHD_GEO_NONE, MFMHD_GEO_NONE },
    { 733,  5, MFM_WD1002_SECTORS, 300,            MFMHD_GEO_NONE },
    { 640,  6, MFM_WD1002_SECTORS, MFMHD_GEO_NONE, MFMHD_GEO_NONE },
    { 1024, 8, MFM_WD1002_SECTORS, 1024,           1024 },
    { 820,  6, MFM_WD1002_SECTORS, MFMHD_GEO_NONE, MFMHD_GEO_NONE },
    { 612,  2, MFM_WD1002_SECTORS, 128,            128 },
    { 612,  4, MFM_WD1002_SECTORS, 128,            MFMHD_GEO_NONE },
    { 306,  4, MFM_WD1002_SECTORS, 0,              MFMHD_GEO_NONE }
};
#endif

static int mfmhd_ioctl(struct inode *inode, struct file *filp,
    unsigned int cmd, unsigned int arg);
static int mfmhd_open(struct inode *inode, struct file *filp);
static void mfmhd_release(struct inode *inode, struct file *filp);
static int INITPROC mfmhd_probe_drive(int drive);
static void mfmhd_recalibrate_drive(unsigned int port, int drive);
static unsigned char mfmhd_default_cmd_control(void);
static unsigned char mfmhd_sanitize_cmd_control(unsigned char cmd_control);
static unsigned char mfmhd_drive_cmd_control(int drive);
static void mfmhd_build_command(unsigned char *cmdblk, unsigned char op,
    unsigned char drive, unsigned char head, unsigned short cylinder,
    unsigned char sector, unsigned char count, unsigned char cmd_control);
static int mfmhd_cmd_raw(unsigned int port);
static int mfmhd_cmd(unsigned int port, unsigned char *cmdblk,
    unsigned char *read, unsigned int rlen, unsigned char *write,
    unsigned int wlen);
static int mfmhd_set_drive_parameters(unsigned int port, int drive);
static int mfmhd_rw_chunk(int drive, sector_t lba, unsigned int sectors,
    char *buffer, int write);
static void mfmhd_write_control(unsigned int port, unsigned char value);
static void mfmhd_dma_stop(void);
static int mfmhd_reset_controller(unsigned int port);
static void mfmhd_resync_controller(unsigned int port, int drive, unsigned char op);

static int mfmhd_initialized;

static struct file_operations mfmhd_fops = {
    NULL,                       /* lseek */
    block_read,                 /* read */
    block_write,                /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    mfmhd_ioctl,                /* ioctl */
    mfmhd_open,                 /* open */
    mfmhd_release               /* release */
};

static struct mfm_drive_info drive_info[MFM_MAX_DRIVES];
static struct drive_infot mfmhd_drive_info[MFM_MAX_DRIVES];
static struct hd_struct mfmhd_part[MFM_MAX_DRIVES << MFM_MINOR_SHIFT];

static struct gendisk mfmhd_gendisk = {
    MAJOR_NR,
    "mfm",
    MFM_MINOR_SHIFT,
    MFM_MAX_PARTITIONS,
    MFM_MAX_DRIVES,
    mfmhd_part,
    0,
    mfmhd_drive_info
};

static int access_count[MFM_MAX_DRIVES];
static unsigned char mfmhd_control_shadow;
static int mfmhd_quiet_probe;
static char *mfmhd_bounce;
extern struct isa_conf mfm_conf;    /* /bootopts mfm=irq,port,,flags */
static int mfmhd_slow_profile;      /* MFMF_SLOW selects slow timing */
/*
 * MFMF_PIO moves sector payloads over programmed I/O instead of
 * 8237 DMA channel 3.  Machines whose chipset does not service DRQ3 the way
 * a true IBM XT does (the Amstrad PC1512/PC1640 among them) need this; the
 * driver also sets it by itself after a failed DMA transfer.
 */
static int mfmhd_pio;
/*
 * MFMF_TRACE.  Tracing is off unless explicitly requested: these
 * printks go to the console, which is the only diagnostic channel that
 * survives a hard lock-up (the dmesg ring is lost with the machine, and the
 * one serial port is taken by the SerDrive root disk).  Whatever printed last
 * before a freeze stays on screen.
 */
static int mfmhd_trace;
/*
 * MFMF_IRQ requests interrupt-driven command completion on mfm_conf.irq, the
 * XT fixed-disk line.  The controller has always been able to do this - bit 1
 * of the port+3 mask register (MFM_CTL_IRQEN) asserts IRQ on command
 * completion - but the driver only ever wrote MFM_CTL_PIO_POLLED and span on
 * the status port instead, which burns the CPU for the whole of every transfer.
 *
 * It is opt-in rather than default because this is the driver the system boots
 * from: if the board does not actually drive the line, or the line is shared,
 * an unbootable machine is the failure mode.  Booting without the bit restores
 * the polled behaviour exactly.  The poll loop is retained underneath as a
 * backstop, so a missing interrupt costs latency rather than a hang.
 */
static int mfmhd_irq_mode;
/*
 * MFMF_EXTWRITE sets the 8237's Extended Write mode (command register bit 5).
 * The command register bit picks the write-strobe timing: 0 is Late Write,
 * 1 is Extended Write, which asserts the strobe a clock earlier and so holds
 * it for longer.  It exists for peripherals that cannot meet the short pulse.
 *
 * The Amstrad PC1512/1640 need it.  Their 8237 is clocked at 4MHz and takes a
 * five-clock 1.25us bus cycle on channels 1-3 (PC1640 Technical Reference
 * 1.5), where a real XT clocks the part near 2.39MHz for a four-clock cycle of
 * about 1.68us - so the Amstrad completes each transfer roughly 25% faster
 * than 8-bit cards were designed against, and the ROS firmware additionally
 * initialises the controller to Late Write (1.5.2), the shorter of the two.
 * The result is dropped or mistimed bytes: distorted audio on the sound card,
 * and a hard disk controller that misses its handshake altogether and forces
 * the driver back to PIO.
 *
 * Off by default: machines with standard timing have no reason to alter a
 * working controller, and the setting is harmless where enabled unnecessarily.
 */
static int mfmhd_extwrite;
static struct wait_queue mfmhd_wait;
static volatile unsigned char mfmhd_irq_seen;
static unsigned char mfmhd_irq_armed;

static jiff_t mfmhd_probe_deadline;     /* 0 once probing is over */

static int
mfmhd_probe_expired(void)
{
    return mfmhd_probe_deadline && time_after(jiffies(), mfmhd_probe_deadline);
}

/*
 * Because the driver is synchronous these scratch blocks are never shared
 * between two in-flight commands.  Keeping them static keeps command- and
 * sense-sized arrays off the kernel stack in every call chain, including
 * the error path where a sense fetch and a recalibrate follow a failed
 * command; those recovery commands run only after the failed command's
 * bytes have all been transferred, so reusing mfmhd_cmdblk is safe there.
 */
static unsigned char mfmhd_cmdblk[MFM_COMMAND_BYTES];
static unsigned char mfmhd_sensecmd[MFM_COMMAND_BYTES];
static unsigned char mfmhd_sense[MFM_SENSE_BYTES];
static unsigned char mfmhd_params[MFM_SETPARAM_PARAMETER_BYTES];

/*
 * Parameters of the controller command currently in flight.  Passing these
 * through a static request block instead of a seven-argument list keeps 14
 * bytes of pushed arguments off the caller's frame on the read/write path.
 * Safe for the same reason as the scratch blocks above: one command at a
 * time, and the driver never reschedules mid-command.
 */
static struct {
    unsigned char *cmdblk;
    unsigned char *read;
    unsigned int rlen;
    unsigned char *write;
    unsigned int wlen;
} cmdreq;

/*
 * Geometry working set for the sector transfer path, static for the same
 * reason: the driver is fully polled and never reschedules, so the chain
 * below do_mfmhd_request() runs to completion and cannot be re-entered.
 * Kernel stack is the scarce resource here, since ELKS statically allocates
 * a stack for every task and this path nests beneath the filesystem.
 */
static struct {
    unsigned int unit;
    unsigned int heads;
    unsigned int sec;
    unsigned int cyl;
    unsigned int max_cyl;
    unsigned int head;
    unsigned int sector;
    sector_t cmd_lba;
    sector_t unit_left;
} rw;

static sector_t INITPROC
mfmhd_get_le_sector(const char *bytes)
{
    union mfm_sector_words result;

    result.word.low = MFM_LE16(bytes, 0);
    result.word.high = MFM_LE16(bytes, 2);
    return result.value;
}

#define mfmhd_debug_set(stage, drive, port, error) ((void)0)

static void
mfmhd_write_control(unsigned int port, unsigned char value)
{
    /* Internal callers may enable DMA; ioctl callers are checked separately. */
    value &= MFM_CTL_VALID_MASK;
    mfmhd_control_shadow = value;
    outb_p(value, port + MFM_HD_CONTROL);
}

/*
 * Return nonzero only for controller operations whose bulk payload is
 * defined to use the XT hard-disk DMA channel.  Parameter, sense, and
 * controller-specific geometry bytes remain programmed-I/O transfers.
 */
static int
mfmhd_command_uses_dma(unsigned char op, unsigned int rlen,
    unsigned int wlen)
{
    if (mfmhd_pio)
        return 0;
    if ((!rlen && !wlen) || (rlen && wlen))
        return 0;

    return op == MFM_CMD_READ_SECTORS || op == MFM_CMD_WRITE_SECTORS;
}

/*
 * Program PC/XT 8237 DMA channel 3 using only byte and word arithmetic.
 * SEG_DMASEG is a physical paragraph number.  Its low DMA address is
 * SEG_DMASEG * 16 and its page byte is SEG_DMASEG / 4096; both calculations
 * are constant 16-bit shifts.  The fixed 1 KiB region cannot wrap at 64 KiB.
 *
 * DMA_MODE_READ means controller-to-memory.  DMA_MODE_WRITE means
 * memory-to-controller.  The 8237 count register stores byte_count - 1.
 */
static int
mfmhd_dma_prepare(unsigned int rlen, unsigned char *write,
    unsigned int wlen)
{
    unsigned int count;
    unsigned int flags;
    unsigned char mode;
    union mfm_dma_word address;
    union mfm_dma_word dma_count;

    count = rlen ? rlen : wlen;
    if (!count || count > MFM_DMA_BYTES || (count & 1U))
        return -1;
    if (wlen && !write)
        return -1;

    if (wlen) {
        fmemcpyw(0, DMASEG, write, kernel_ds, count >> 1);
        mode = DMA_MODE_WRITE;
    } else {
        mode = DMA_MODE_READ;
    }

    address.word = (unsigned int)(SEG_DMASEG << 4);
    dma_count.word = count - 1U;

    /* No interrupt may disturb the shared 8237 address/count flip-flop. */
    save_flags(flags);
    clr_irq();
    outb(MFM_DMA_CHANNEL | 4, DMA1_MASK_REG);
    outb(0, DMA1_CLEAR_FF_REG);
    outb(mode | MFM_DMA_CHANNEL, DMA1_MODE_REG);
    outb(address.byte[0], MFM_DMA_ADDRESS_PORT);
    outb(address.byte[1], MFM_DMA_ADDRESS_PORT);
    outb((unsigned char)(SEG_DMASEG >> 12), MFM_DMA_PAGE_PORT);
    outb(dma_count.byte[0], MFM_DMA_COUNT_PORT);
    outb(dma_count.byte[1], MFM_DMA_COUNT_PORT);
    outb(MFM_DMA_CHANNEL, DMA1_MASK_REG);
    restore_flags(flags);
    return 0;
}

static void
mfmhd_dma_stop(void)
{
    unsigned int flags;

    save_flags(flags);
    clr_irq();
    outb(MFM_DMA_CHANNEL | 4, DMA1_MASK_REG);
    restore_flags(flags);
}

static void
mfmhd_dma_copy_read(unsigned char *read, unsigned int count)
{
    if (read)
        fmemcpyw(read, kernel_ds, 0, DMASEG, count >> 1);
}

static unsigned char
mfmhd_sanitize_cmd_control(unsigned char cmd_control)
{
    return (unsigned char)(cmd_control &
        (MFM_CCB_R1_NO_RETRY | MFM_CCB_R2_IMMEDIATE_ECC |
         MFM_CCB_STEP_MASK));
}

static unsigned char
mfmhd_default_cmd_control(void)
{
    unsigned char cmd_control;

    cmd_control = (unsigned char)(MFMHD_STEP_CODE & MFM_CCB_STEP_MASK);
#if MFMHD_NO_RETRY
    cmd_control |= MFM_CCB_R1_NO_RETRY;
#endif
#if MFMHD_ECC_IMMEDIATE
    cmd_control |= MFM_CCB_R2_IMMEDIATE_ECC;
#endif
    if (mfmhd_slow_profile)
        cmd_control &= ~MFM_CCB_STEP_MASK;
    return mfmhd_sanitize_cmd_control(cmd_control);
}

static void INITPROC
mfmhd_init_drive_state(int drive)
{
    drive_info[drive].cmd_control = mfmhd_default_cmd_control();
    drive_info[drive].last_csb = 0;
    memset(drive_info[drive].last_sense, 0, MFM_SENSE_BYTES);
}

static unsigned char
mfmhd_drive_cmd_control(int drive)
{
    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return mfmhd_default_cmd_control();
    if (mfmhd_slow_profile)
        drive_info[drive].cmd_control &= ~MFM_CCB_STEP_MASK;
    return mfmhd_sanitize_cmd_control(drive_info[drive].cmd_control);
}


static const char * INITPROC
mfmhd_bios_name(void)
{
#if MFMHD_WD1002_BIOS == MFMHD_BIOS_G
    return "G";
#elif MFMHD_WD1002_BIOS == MFMHD_BIOS_H
    return "H";
#else
    return "Super";
#endif
}

static const char * INITPROC
mfmhd_source_name(unsigned int source)
{
    switch (source) {
    case MFM_GEO_SRC_USER:
        return "user";
    case MFM_GEO_SRC_WD1002:
        return "wd1002";
    case MFM_GEO_SRC_BPB:
        return "bpb";
    case MFM_GEO_SRC_SEAGATE:
        return "seagate";
    case MFM_GEO_SRC_FILECARD20:
        return "filecard20";
    default:
        return "unknown";
    }
}

static int
mfmhd_is_none(unsigned short value)
{
    return value == MFMHD_GEO_NONE;
}

static void INITPROC
mfmhd_print_cylinder_value(const char *name, unsigned short value)
{
    if (mfmhd_is_none(value))
        printk(" %s=none", name);
    else
        printk(" %s=%u", name, value);
}

static void INITPROC
mfmhd_init_ports(void)
{
    mfmhd_control_shadow = MFM_CTL_PIO_POLLED;

    /*
     * WD1002 BIOS ROMs may leave the adapter in interrupt/DMA mode; put the
     * controller into the polled-PIO state before the first command.
     */
    mfmhd_write_control(mfm_port, MFM_CTL_PIO_POLLED);

    if (mfmhd_controller_is_seagate())
        printk("mfmhd: Seagate ST11M/R/FileCard profile, port 0x%x\n",
            mfm_port);
    else
        printk("mfmhd: WD1002A-WX1 profile, BIOS table=%s, port 0x%x\n",
            mfmhd_bios_name(), mfm_port);
    if (mfmhd_slow_profile)
        printk("mfmhd: slow timing profile enabled\n");
}

static int INITPROC
mfmhd_jumper_is_open(unsigned int raw, int s1_number)
{
    unsigned int mask;

    mask = 1U << (s1_number - 1);
#if MFMHD_JUMPER_OPEN_IS_ONE
    return (raw & mask) != 0;
#else
    return (raw & mask) == 0;
#endif
}

/*
 * Calculate heads * sectors.  Controller geometry limits make the normal
 * result at most 1024.  Zero reports either invalid input or 16-bit
 * overflow; callers reject both cases.
 */
static unsigned int
mfmhd_sectors_per_cylinder(unsigned int heads, unsigned int sectors)
{
    unsigned long spc = (unsigned long)heads * sectors;

    return (spc > 0xffffUL)? 0: (unsigned int)spc;
}

/*
 * Total sector capacity of a CHS geometry.  The widening multiply cannot
 * overflow: the largest 16-bit operands give 0xffff * 0xffff = 0xfffe0001,
 * which still fits a sector_t.  Zero reports invalid geometry.
 */
static sector_t INITPROC
mfmhd_chs_capacity(unsigned int cylinders, unsigned int heads,
    unsigned int sectors)
{
    unsigned int spc = mfmhd_sectors_per_cylinder(heads, sectors);

    return spc? (sector_t)cylinders * spc: 0;
}

static int INITPROC
mfmhd_geometry_sane(unsigned int cyl, unsigned int head, unsigned int sec,
    sector_t totalsz)
{
    sector_t span;

    if (!cyl || !head || !sec)
        return 0;
    if (cyl > MFM_WD1002_MAX_CYLINDERS)
        return 0;
    if (head > MFM_WD1002_MAX_HEADS)
        return 0;
    if (sec > 64)
        return 0;

    span = mfmhd_chs_capacity(cyl, head, sec);
    if (!span)
        return 0;
    if (totalsz && totalsz > span)
        return 0;

    return 1;
}

static int
mfmhd_lba_to_chs(sector_t lba, unsigned int heads, unsigned int sec,
    unsigned int *pcyl, unsigned int *phead, unsigned int *psector)
{
    sector_t quotient;
    unsigned int spc;
    unsigned int tmp;

    spc = mfmhd_sectors_per_cylinder(heads, sec);
    if (!spc)
        return -1;

    /* __divmod divides the sector_t word pair, returning the remainder in tmp */
    tmp = spc;
    quotient = __divmod(lba, &tmp);
    if (quotient > 0xffffUL)            /* cannot reach the 16-bit CHS interface */
        return -1;
    *pcyl = (unsigned int)quotient;

    *phead = tmp / sec;
    *psector = tmp - (*phead * sec);
    return 0;
}

static void INITPROC
mfmhd_set_drive_geometry(int drive, const struct mfm_wd1002_geom *entry,
    unsigned int source)
{
    drive_info[drive].cylinders = entry->cylinders;
    drive_info[drive].heads = entry->heads;
    drive_info[drive].sectors = entry->sectors ?
        entry->sectors : MFM_WD1002_SECTORS;
    drive_info[drive].wp_cylinder = entry->wp_cylinder;
    drive_info[drive].rwc_cylinder = entry->rwc_cylinder;
    drive_info[drive].source = (unsigned char)source;
}

static int INITPROC
mfmhd_set_geometry_from_user(int drive)
{
    unsigned int cyl;
    unsigned int heads;
    unsigned int sec;
    struct mfm_wd1002_geom entry;

    if (drive == 0) {
        cyl = MFMHD_DRIVE0_CYLINDERS;
        heads = MFMHD_DRIVE0_HEADS;
        sec = MFMHD_DRIVE0_SECTORS;
    } else {
        cyl = MFMHD_DRIVE1_CYLINDERS;
        heads = MFMHD_DRIVE1_HEADS;
        sec = MFMHD_DRIVE1_SECTORS;
    }

    if (!cyl && !heads && !sec)
        return 0;
    if (cyl && heads && !sec)
        sec = MFM_WD1002_SECTORS;

    if (!mfmhd_geometry_sane(cyl, heads, sec, 0)) {
        printk("mfmhd: /dev/mfm%c invalid user geometry %u/%u/%u ignored\n",
            'a' + (unsigned char)drive, cyl, heads, sec);
        return 0;
    }

    entry.cylinders = (unsigned short)cyl;
    entry.heads = (unsigned char)heads;
    entry.sectors = (unsigned char)sec;
    entry.wp_cylinder = MFMHD_GEO_NONE;
    entry.rwc_cylinder = MFMHD_GEO_NONE;
    mfmhd_set_drive_geometry(drive, &entry, MFM_GEO_SRC_USER);

    printk("mfmhd: /dev/mfm%c using user geometry %u/%u/%u\n",
        'a' + (unsigned char)drive, cyl, heads, sec);
    return 1;
}

static const struct mfm_wd1002_geom * INITPROC
mfmhd_table_entry(unsigned int type_index, unsigned int *table_size)
{
#if MFMHD_WD1002_BIOS == MFMHD_BIOS_G
    *table_size = sizeof(mfmhd_wd1002_g_table) /
        sizeof(mfmhd_wd1002_g_table[0]);
    if (type_index >= *table_size)
        type_index = 0;
    return &mfmhd_wd1002_g_table[type_index];
#elif MFMHD_WD1002_BIOS == MFMHD_BIOS_H
    *table_size = sizeof(mfmhd_wd1002_h_table) /
        sizeof(mfmhd_wd1002_h_table[0]);
    if (type_index >= *table_size)
        type_index = 0;
    return &mfmhd_wd1002_h_table[type_index];
#else
    *table_size = sizeof(mfmhd_wd1002_super_table) /
        sizeof(mfmhd_wd1002_super_table[0]);
    if (type_index >= *table_size)
        type_index = 0;
    return &mfmhd_wd1002_super_table[type_index];
#endif
}

static unsigned int INITPROC
mfmhd_wd1002_type_from_s1(int drive, unsigned int raw_s1)
{
    unsigned int type_index;

#if MFMHD_WD1002_BIOS == MFMHD_BIOS_H
    if (drive & 1) {
        /* Revision H: S1/1, S1/2 and S1/7 configure drive 1. */
        type_index = (unsigned int)mfmhd_jumper_is_open(raw_s1, 1);
        type_index |= (unsigned int)mfmhd_jumper_is_open(raw_s1, 2) << 1;
        type_index |= (unsigned int)mfmhd_jumper_is_open(raw_s1, 7) << 2;
    } else {
        /* Revision H: S1/3, S1/4 and S1/8 configure drive 0. */
        type_index = (unsigned int)mfmhd_jumper_is_open(raw_s1, 3);
        type_index |= (unsigned int)mfmhd_jumper_is_open(raw_s1, 4) << 1;
        type_index |= (unsigned int)mfmhd_jumper_is_open(raw_s1, 8) << 2;
    }
#else
    if (drive & 1) {
        /* Super/G: S1/2 and S1/4 configure drive 1. */
        type_index = (unsigned int)mfmhd_jumper_is_open(raw_s1, 2);
        type_index |= (unsigned int)mfmhd_jumper_is_open(raw_s1, 4) << 1;
    } else {
        /* Super/G: S1/1 and S1/3 configure drive 0. */
        type_index = (unsigned int)mfmhd_jumper_is_open(raw_s1, 1);
        type_index |= (unsigned int)mfmhd_jumper_is_open(raw_s1, 3) << 1;
    }
#endif
    return type_index;
}

static void INITPROC
mfmhd_set_wd1002_geometry_by_jumper(int drive, unsigned int port)
{
    const struct mfm_wd1002_geom *entry;
    unsigned int raw_s1;
    unsigned int type_index;
    unsigned int table_size;

    raw_s1 = (unsigned int)inb_p(port + MFM_HD_JUMPER) & 0xff;
    type_index = mfmhd_wd1002_type_from_s1(drive, raw_s1);
    entry = mfmhd_table_entry(type_index, &table_size);
    if (type_index >= table_size)
        type_index = 0;

    mfmhd_set_drive_geometry(drive, entry, MFM_GEO_SRC_WD1002);

    printk("mfmhd: /dev/mfm%c WD1002A-WX1 %s type %u rawS1=0x%02x geometry %u/%u/%u",
        'a' + (unsigned char)drive, mfmhd_bios_name(), type_index,
        raw_s1, drive_info[drive].cylinders, drive_info[drive].heads,
        drive_info[drive].sectors);
    mfmhd_print_cylinder_value("wp", drive_info[drive].wp_cylinder);
    mfmhd_print_cylinder_value("rwc", drive_info[drive].rwc_cylinder);
    printk("\n");
}

static void
mfmhd_dump_status(unsigned int port, const char *where, unsigned char status)
{
    printk("mfmhd: %s port=0x%x status=%02x (irq=%d req=%d sel=%d cmd=%d in=%d ready=%d)\n",
        where, port, status,
        (status & MFM_STAT_INTERRUPT) != 0,
        (status & MFM_STAT_REQUEST) != 0,
        (status & MFM_STAT_SELECT) != 0,
        (status & MFM_STAT_COMMAND) != 0,
        (status & MFM_STAT_INPUT) != 0,
        (status & MFM_STAT_READY) != 0);
}

/*
 * Completion interrupt.  The controller latches MFM_STAT_INTERRUPT in the
 * status port; reading status is what the polled path already does, so the
 * flag is simply recorded and the sleeper woken.  The PIC end-of-interrupt is
 * issued by the irqit wrapper, so the PIC is not touched here.
 */
static void
mfmhd_interrupt(int irq, struct pt_regs *regs)
{
    (void)irq;
    (void)regs;
    mfmhd_irq_seen = 1;
    wake_up(&mfmhd_wait);
}

/*
 * Wait for command completion.  With mfm= bit 3 the task sleeps until the
 * controller's interrupt arrives, giving the CPU back for the duration of the
 * transfer; the caller's own timeout still bounds the wait, and the polled
 * loop below still runs afterwards to confirm the status bits, so an
 * interrupt that never arrives degrades to the old behaviour rather than
 * hanging.
 */
static void
mfmhd_sleep_for_irq(jiff_t timeout)
{
    if (!mfmhd_irq_armed)
        return;
    prepare_to_wait_interruptible(&mfmhd_wait);
    if (!mfmhd_irq_seen && !current->signal) {
        current->timeout = jiffies() + timeout + 1;
        do_wait();
        current->timeout = 0;
    }
    finish_wait(&mfmhd_wait);
}

static int
mfmhd_wait_status(unsigned int port, unsigned char flags, unsigned char mask,
    jiff_t timeout, const char *where)
{
    jiff_t expiry;
    unsigned char st;
    unsigned int spins;
    unsigned int rounds;

    expiry = jiffies() + timeout;
    st = 0;
    spins = 0;
    rounds = 0;
    do {
        st = STATUS(port);
        if (st == 0xff)
            return -1;
        if ((st & mask) == flags)
            return 0;
        if (++spins == 0 && ++rounds >= MFM_SPIN_ROUNDS)
            break;              /* jiffies may be frozen; never spin forever */
    } while (!time_after(jiffies(), expiry));

    mfmhd_dump_status(port, where, st);
    return -1;
}

static int
mfmhd_command_expects_data_phase(unsigned char op)
{
    return op == MFM_CMD_READ_SECTORS || op == MFM_CMD_WRITE_SECTORS ||
        op == MFM_CMD_SENSE || op == MFM_CMD_ST11_GET_GEOMETRY;
}

static jiff_t
mfmhd_phase_ticks(void)
{
    return mfmhd_slow_profile ? MFM_SLOW_PHASE_TICKS : MFM_PHASE_TICKS;
}

static jiff_t
mfmhd_select_ticks(void)
{
    return mfmhd_slow_profile ? MFM_SLOW_SELECT_TICKS : MFM_SELECT_TICKS;
}

static jiff_t
mfmhd_command_wait_ticks(unsigned char op)
{
    switch (op) {
    case MFM_CMD_RECALIBRATE:
    case MFM_CMD_SEEK:              /* a park is a full stroke, 3ms a step */
    case MFM_CMD_READ_SECTORS:
    case MFM_CMD_WRITE_SECTORS:
    case MFM_CMD_INIT_DRIVE_PARAMETERS:
    case MFM_CMD_ST11_GET_GEOMETRY:
        return mfmhd_slow_profile ? MFM_SLOW_DISK_TICKS : MFM_DISK_TICKS;
    default:
        return mfmhd_phase_ticks();
    }
}

/*
 * Recovery commands (recalibrate) are issued through the full mfmhd_cmd()
 * path below.  That cannot recurse further: a recalibrate has no data
 * phase, so a failing recalibrate resyncs without another recalibrate.
 */
static void
mfmhd_resync_controller(unsigned int port, int drive, unsigned char op)
{
    /* Drop select/control to a known polled-PIO state before the next command. */
    mfmhd_dma_stop();
    outb_p(0, port + MFM_HD_SELECT);
    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
    /* Data ops benefit most from a recalibrate after phase faults. */
    if (drive >= 0 && mfmhd_command_expects_data_phase(op))
        mfmhd_recalibrate_drive(port, drive);
}

static int
mfmhd_drive_from_unit(unsigned char unit)
{
#if MFMHD_CONTROLLER == MFMHD_CTRL_SEAGATE && MFMHD_DRIVES == 1
    /*
     * FileCard 20 presents both physical controller units as one logical
     * /dev/mfma.  Attribute second-half completions to that logical drive.
     */
    if (drive_info[0].source == MFM_GEO_SRC_FILECARD20)
        return 0;
#endif
    return (int)(unit & 0x01);
}

static void
mfmhd_note_completion(int drive, unsigned char csb, unsigned char *sense)
{
    drive_info[drive].last_csb = csb;
    if (sense)
        memcpy(drive_info[drive].last_sense, sense, MFM_SENSE_BYTES);
}

static unsigned int
mfmhd_data_in_burst(unsigned int port, unsigned char **pread,
    unsigned int count)
{
    unsigned char *buf;
    unsigned char st;
    unsigned int done;

    buf = pread ? *pread : NULL;
    done = 0;
    if (buf) {
        do {
            *buf++ = DATA(port);
            done++;
            if (--count == 0)
                break;
            st = STATUS(port);
        } while ((st & (MFM_STAT_READY | MFM_STAT_PHASE_MASK)) ==
            (MFM_STAT_READY | MFM_PHASE_DATA_IN));
    } else {
        do {
            (void)DATA(port);
            done++;
            if (--count == 0)
                break;
            st = STATUS(port);
        } while ((st & (MFM_STAT_READY | MFM_STAT_PHASE_MASK)) ==
            (MFM_STAT_READY | MFM_PHASE_DATA_IN));
    }
    if (pread)
        *pread = buf;
    return done;
}

static unsigned int
mfmhd_data_out_burst(unsigned int port, unsigned char **pwrite,
    unsigned int count)
{
    unsigned char *buf;
    unsigned char st;
    unsigned int done;

    buf = *pwrite;
    done = 0;
    do {
        DATA_OUT(port, *buf++);
        done++;
        if (--count == 0)
            break;
        st = STATUS(port);
    } while ((st & (MFM_STAT_READY | MFM_STAT_PHASE_MASK)) ==
        (MFM_STAT_READY | MFM_PHASE_DATA_OUT));
    *pwrite = buf;
    return done;
}

/*
 * mfmhd_cmd_raw() return values: the completion status byte, with
 * MFM_CMD_EARLY_STATUS set when the controller reported status before all
 * bytes moved, or a negative code.  No recovery command is ever issued from
 * inside the phase engine; sense collection, resync, and recalibration all
 * happen sequentially in mfmhd_cmd() after this frame has returned, so
 * phase-engine stack frames never nest.
 */
#define MFM_CMD_EARLY_STATUS    0x100
#define MFM_CMD_FAULT           (-1)    /* handshake fault: count + resync */
#define MFM_CMD_NOSELECT        (-2)    /* never selected: quiet cleanup */

/*
 * A phase fault means the driver and the controller disagree about where they
 * are in the command protocol, and the controller does not leave that state on
 * its own: it sits with SELECT asserted (status 0xCB, drive light stuck on) and
 * every later command fails, so a probe after one reports "no XT MFM drives
 * found" on a perfectly good disk.  Worse, a fault part way through a write
 * leaves the sector half written, which is how a file on this disk ends up
 * truncated.
 *
 * Pulsing the reset port clears it - measured on a wedged WD1002A-WX1, status
 * went from 0xCB straight back to 0xC0 - so recover here rather than leaving
 * the board stuck for whatever runs next.  The command still fails and the
 * caller still retries; this only ensures the retry meets a sane controller.
 */
static int
mfmhd_phase_fault(unsigned int port, const char *where, unsigned char st)
{
    unsigned char after;
    int i;

    mfmhd_dump_status(port, where, st);
    mfmhd_debug_set(40, -1, port, -1);

    outb_p(1, port + MFM_HD_RESET);
    for (i = 0; i < 1000; i++) {        /* the board needs a moment to settle */
        after = STATUS(port);
        if (!(after & MFM_STAT_SELECT))
            break;
    }
    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
    if (after & MFM_STAT_SELECT)
        printk("mfmhd: controller still busy after reset, status=%02x\n", after);
    else if (mfmhd_trace)
        printk("mfmhd: controller reset after %s, status=%02x\n", where, after);

    return MFM_CMD_FAULT;
}

static int
mfmhd_cmd_raw(unsigned int port)
{
    unsigned char op;
    unsigned int cmdleft;
    unsigned int inleft;
    unsigned int outleft;
    unsigned int dummy_out;
    unsigned int done_count;
    jiff_t phase_deadline;
    unsigned char phase;
    unsigned char st;
    unsigned char csb;
    unsigned char early_status;
    unsigned char dma_active;
    unsigned int spins;
    unsigned int rounds;

    if (!cmdreq.cmdblk)
        return MFM_CMD_NOSELECT;

    op = cmdreq.cmdblk[0];
    cmdleft = MFM_COMMAND_BYTES;
    inleft = cmdreq.rlen;
    outleft = cmdreq.wlen;
    dma_active = (unsigned char)mfmhd_command_uses_dma(op, cmdreq.rlen,
        cmdreq.wlen);
    dummy_out = 0;
    csb = 0xff;
    early_status = 0;


    if (dma_active) {
        if (mfmhd_dma_prepare(cmdreq.rlen, cmdreq.write, cmdreq.wlen))
            return MFM_CMD_NOSELECT;
        /* The 8237 owns the payload; the phase loop handles only the CDB/CSB. */
        inleft = 0;
        outleft = 0;
    }

    /* Match Linux xd WD1002 select-then-control ordering. */
    outb_p(0, port + MFM_HD_SELECT);
    /*
     * Clear the seen-flag before the controller can raise the line, or a
     * stale interrupt from the previous command would satisfy this one's wait.
     */
    mfmhd_irq_seen = 0;
    mfmhd_write_control(port,
        (unsigned char)((dma_active ? MFM_CTL_DRQEN : MFM_CTL_PIO_POLLED) |
                        (mfmhd_irq_armed ? MFM_CTL_IRQEN : 0)));

    if (mfmhd_trace)
        printk("mfmhd: cmd op=%02x dma=%d in=%u out=%u\n", op, dma_active,
            inleft, outleft);

    if (mfmhd_wait_status(port, MFM_STAT_SELECT, MFM_STAT_SELECT,
            mfmhd_select_ticks(), "select/busy timeout"))
        return MFM_CMD_NOSELECT;

    phase_deadline = jiffies() + mfmhd_phase_ticks();
    spins = 0;
    rounds = 0;
    while (1) {
        st = STATUS(port);
        if (st == 0xff)
            return mfmhd_phase_fault(port, "floating bus", st);

        if (!(st & MFM_STAT_READY)) {
            /* See MFM_SPIN_ROUNDS: jiffies may not be advancing here. */
            if (++spins == 0) {
                /*
                 * Heartbeat: one line per 65536 status reads.  A phase engine
                 * stuck in a loop shows up as this line repeating, naming the
                 * phase it cannot leave - which a post-mortem of a frozen
                 * screen cannot otherwise reveal.
                 */
                if (mfmhd_trace)
                    printk("mfmhd: spin notready st=%02x round=%u left c%u i%u o%u\n",
                        st, rounds, cmdleft, inleft, outleft);
                if (++rounds >= MFM_SPIN_ROUNDS)
                    return mfmhd_phase_fault(port, "request timeout (spin)", st);
            }
            if (time_after(jiffies(), phase_deadline))
                return mfmhd_phase_fault(port, "request timeout", st);
            continue;
        }

        phase = st & MFM_STAT_PHASE_MASK;

        /*
         * In DMA mode the 8237 owns the payload, so the data phases give this
         * loop nothing to do.  READY asserted in a data phase must therefore
         * not count as forward progress: a controller that parks there (the
         * PC1640's does, when it never drives DRQ3) would otherwise reset the
         * backstop on every pass, and phase_deadline is only consulted on the
         * !READY path above - spinning here forever, silently, with interrupts
         * masked.  Age the same backstop instead so the transfer can fail.
         */
        if (dma_active && (phase == MFM_PHASE_DATA_IN ||
                phase == MFM_PHASE_DATA_OUT)) {
            if (++spins == 0) {
                if (mfmhd_trace)
                    printk("mfmhd: spin dmadata st=%02x round=%u\n", st, rounds);
                if (++rounds >= MFM_SPIN_ROUNDS)
                    return mfmhd_phase_fault(port, "dma stalled in data phase", st);
            }
            if (time_after(jiffies(), phase_deadline))
                return mfmhd_phase_fault(port, "dma phase timeout", st);
            continue;
        }

        spins = 0;
        rounds = 0;             /* progress: restart the backstop */

        switch (phase) {
        case MFM_PHASE_CMD_OUT:
            /* C/D=1 I/O=0: command block bytes, host to controller. */
            if (!cmdleft)
                return mfmhd_phase_fault(port,
                    "unexpected command-out phase", st);
            DATA_OUT(port, *cmdreq.cmdblk++);
            cmdleft--;
            phase_deadline = jiffies() + (cmdleft ?
                mfmhd_phase_ticks() : mfmhd_command_wait_ticks(op));
            break;

        case MFM_PHASE_DATA_OUT:
            /* C/D=0 I/O=0: write payload, host to controller.  DMA-mode data
             * phases are handled above and never reach this arm. */
            if (cmdleft || !outleft) {
                /*
                 * The Linux xd driver writes a harmless zero whenever an XT
                 * controller asks for data without a host payload.  Some
                 * WD/Xebec cards use this to drain/reset their bus phase.
                 */
                if (++dummy_out > MFM_COMMAND_BYTES)
                    return mfmhd_phase_fault(port,
                        "unexpected data-out phase", st);
                DATA_OUT(port, 0);
                phase_deadline = jiffies() + mfmhd_phase_ticks();
                break;
            }
            done_count = mfmhd_data_out_burst(port, &cmdreq.write, outleft);
            outleft -= done_count;
            phase_deadline = jiffies() + (outleft ?
                mfmhd_phase_ticks() : mfmhd_command_wait_ticks(op));
            break;

        case MFM_PHASE_DATA_IN:
            /* C/D=0 I/O=1: read payload, controller to host.  DMA-mode data
             * phases are handled above and never reach this arm. */
            if (cmdleft || outleft)
                return mfmhd_phase_fault(port,
                    "data-in before output complete", st);
            if (!inleft)
                return mfmhd_phase_fault(port,
                    "unexpected data-in phase", st);
            done_count = mfmhd_data_in_burst(port,
                cmdreq.read ? &cmdreq.read : NULL, inleft);
            inleft -= done_count;
            phase_deadline = jiffies() + mfmhd_phase_ticks();
            break;

        case MFM_PHASE_STATUS_IN:
            /* C/D=1 I/O=1: command completion byte. */
            if (cmdleft || outleft || inleft) {
                printk("mfmhd: early status op=0x%02x cmdleft=%u inleft=%u outleft=%u\n",
                    op, cmdleft, inleft, outleft);
                early_status = 1;
            }
            csb = DATA(port);
            goto done;
        }
    }

done:
    if (mfmhd_trace)
        printk("mfmhd: end csb=%02x early=%d\n", csb, early_status);

    if (dma_active)
        mfmhd_dma_stop();

    /* Give the CPU up until the controller says it has finished. */
    mfmhd_sleep_for_irq(mfmhd_select_ticks());

    if (mfmhd_wait_status(port, 0, MFM_STAT_SELECT, mfmhd_select_ticks(),
            "busy clear timeout"))
        return MFM_CMD_FAULT;
    if (mfmhd_irq_armed)
        mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
    else if (dma_active)
        mfmhd_write_control(port, MFM_CTL_PIO_POLLED);

    return (int)csb | (early_status ? MFM_CMD_EARLY_STATUS : 0);
}

/*
 * Run one command with recovery.  On a controller-reported error the sense
 * bytes are fetched with a raw command (no nested recovery), then the
 * controller is resynced, recalibrating after failed data operations.
 */
static int
mfmhd_cmd(unsigned int port, unsigned char *cmdblk, unsigned char *read,
    unsigned int rlen, unsigned char *write, unsigned int wlen)
{
    unsigned char op;
    unsigned char csb;
    int drive;
    int result;
    int used_dma;

    op = cmdblk[0];
    drive = mfmhd_drive_from_unit((unsigned char)((cmdblk[1] >> 5) & 0x07));
    used_dma = mfmhd_command_uses_dma(op, rlen, wlen);
    cmdreq.cmdblk = cmdblk;
    cmdreq.read = read;
    cmdreq.rlen = rlen;
    cmdreq.write = write;
    cmdreq.wlen = wlen;
    result = mfmhd_cmd_raw(port);
    if (result < 0) {
        if (result == MFM_CMD_FAULT) {
            /*
             * A handshake fault on a DMA payload usually means the machine
             * never serviced DRQ3.  Switch to programmed I/O for all
             * further transfers so the caller's retry can succeed.
             */
            if (used_dma) {
                mfmhd_pio = 1;
                printk("mfmhd: DMA transfer failed, using PIO from now on\n");
            }
            drive_info[drive].cmd_error_count++;
            mfmhd_resync_controller(port, drive, op);
        } else {
            /* Never selected (or DMA setup failed): quiet cleanup only. */
            mfmhd_dma_stop();
            outb_p(0, port + MFM_HD_SELECT);
            mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
        }
        return -1;
    }

    csb = (unsigned char)result;
    if (op == MFM_CMD_SENSE && read)
        mfmhd_note_completion(drive, csb, read);
    else
        mfmhd_note_completion(drive, csb, NULL);

    if ((result & MFM_CMD_EARLY_STATUS) && !(csb & MFM_CSB_ERROR)) {
        drive_info[drive].cmd_error_count++;
        mfmhd_resync_controller(port, drive, op);
        return -1;
    }

    if (csb & MFM_CSB_ERROR) {
        if (mfmhd_quiet_probe && op == MFM_CMD_TEST_DRIVE_READY) {
            mfmhd_debug_set(42, drive, port, (int)csb);
            mfmhd_resync_controller(port, drive, op);
            return -1;
        }
        if (op != MFM_CMD_SENSE) {
            mfmhd_build_command(mfmhd_sensecmd, MFM_CMD_SENSE,
                (unsigned char)((csb & MFM_CSB_LUN) >> 5), 0, 0, 0, 0,
                mfmhd_drive_cmd_control(drive));
            cmdreq.cmdblk = mfmhd_sensecmd;
            cmdreq.read = mfmhd_sense;
            cmdreq.rlen = MFM_SENSE_BYTES;
            cmdreq.write = NULL;
            cmdreq.wlen = 0;
            result = mfmhd_cmd_raw(port);
            /* A short (early-status) sense leaves stale bytes: reject it. */
            if (result < 0 || (result & (MFM_CSB_ERROR | MFM_CMD_EARLY_STATUS)))
                printk("mfmhd: sense command failed after op=0x%02x\n", op);
            else {
                mfmhd_note_completion(drive, csb, mfmhd_sense);
                printk("mfmhd: sense op=0x%02x bytes=%02x %02x %02x %02x\n",
                    op, mfmhd_sense[0], mfmhd_sense[1], mfmhd_sense[2],
                    mfmhd_sense[3]);
            }
        }
        printk("mfmhd: command 0x%02x failed csb=0x%02x port=0x%x\n",
            op, csb, port);
        mfmhd_debug_set(41, drive, port, (int)csb);
        drive_info[drive].cmd_error_count++;
        mfmhd_resync_controller(port, drive, op);
        return -1;
    }

    if (rlen && mfmhd_command_uses_dma(op, rlen, wlen))
        mfmhd_dma_copy_read(read, rlen);

    return 0;
}

static void
mfmhd_build_command(unsigned char *cmdblk, unsigned char op, unsigned char drive,
    unsigned char head, unsigned short cylinder, unsigned char sector,
    unsigned char count, unsigned char cmd_control)
{
    cmdblk[0] = op;
    cmdblk[1] = (unsigned char)(((drive & 0x01) << 5) | (head & 0x1f));
    cmdblk[2] = (unsigned char)(((cylinder & 0x300) >> 2) | (sector & 0x3f));
    cmdblk[3] = (unsigned char)(cylinder & 0xff);
    cmdblk[4] = count;
    cmdblk[5] = mfmhd_sanitize_cmd_control(cmd_control);
}

static unsigned short
mfmhd_param_cylinder(unsigned short value)
{
    if (mfmhd_is_none(value))
        return 0;
    if (value > MFM_WD1002_MAX_CYLINDERS)
        return MFM_WD1002_MAX_CYLINDERS;
    return value;
}

static int
mfmhd_set_drive_parameters(unsigned int port, int drive)
{
    unsigned short rwc_cylinder;
    unsigned short wp_cylinder;

    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return -1;
    if (!drive_info[drive].heads || !drive_info[drive].cylinders)
        return -1;

    rwc_cylinder = mfmhd_param_cylinder(drive_info[drive].rwc_cylinder);
    wp_cylinder = mfmhd_param_cylinder(drive_info[drive].wp_cylinder);

    mfmhd_build_command(mfmhd_cmdblk, MFM_CMD_INIT_DRIVE_PARAMETERS,
        (unsigned char)drive, 0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    mfmhd_params[0] = (unsigned char)((drive_info[drive].cylinders >> 8) & 0x03);
    mfmhd_params[1] = (unsigned char)(drive_info[drive].cylinders & 0xff);
    mfmhd_params[2] = (unsigned char)(drive_info[drive].heads & 0x1f);
    mfmhd_params[3] = (unsigned char)((rwc_cylinder >> 8) & 0x03);
    mfmhd_params[4] = (unsigned char)(rwc_cylinder & 0xff);
    mfmhd_params[5] = (unsigned char)(wp_cylinder >> 8) & 0x03;
    mfmhd_params[6] = (unsigned char)(wp_cylinder & 0xff);
    mfmhd_params[7] = MFM_DEFAULT_ECC_LENGTH;

    return mfmhd_cmd(port, mfmhd_cmdblk, NULL, 0, mfmhd_params,
        MFM_SETPARAM_PARAMETER_BYTES);
}

static int INITPROC
mfmhd_set_geometry_from_seagate(int drive, unsigned int port)
{
    struct mfm_wd1002_geom entry;
    char *buf;
    unsigned int cyl;
    unsigned int heads;
    unsigned int sec;
    int err;

    buf = (char *)heap_alloc(MFM_SECTOR_BYTES, HEAP_TAG_DRVR);
    if (!buf) {
        printk("mfmhd: unable to allocate Seagate geometry buffer\n");
        return 0;
    }

    mfmhd_build_command(mfmhd_cmdblk, MFM_CMD_ST11_GET_GEOMETRY,
        (unsigned char)drive, 0, 0, 0, 1, 0);
    err = mfmhd_cmd(port, mfmhd_cmdblk, (unsigned char *)buf,
        MFM_SECTOR_BYTES, NULL, 0);
    if (err) {
        printk("mfmhd: /dev/mfm%c Seagate geometry command failed\n",
            'a' + (unsigned char)drive);
        heap_free(buf);
        return 0;
    }

    cyl = ((unsigned int)(unsigned char)buf[0x02] << 8) |
        (unsigned int)(unsigned char)buf[0x03];
    heads = (unsigned int)(unsigned char)buf[0x04];
    sec = (unsigned int)(unsigned char)buf[0x05];
    heap_free(buf);

    if (!mfmhd_geometry_sane(cyl, heads, sec, 0)) {
        printk("mfmhd: /dev/mfm%c Seagate geometry %u/%u/%u unusable\n",
            'a' + (unsigned char)drive, cyl, heads, sec);
        return 0;
    }

    entry.cylinders = (unsigned short)cyl;
    entry.heads = (unsigned char)heads;
    entry.sectors = (unsigned char)sec;
    entry.wp_cylinder = MFMHD_GEO_NONE;
    entry.rwc_cylinder = MFMHD_GEO_NONE;
    mfmhd_set_drive_geometry(drive, &entry, MFM_GEO_SRC_SEAGATE);

    printk("mfmhd: /dev/mfm%c Seagate controller geometry %u/%u/%u\n",
        'a' + (unsigned char)drive, cyl, heads, sec);
    return 1;
}

static int INITPROC
mfmhd_set_geometry_from_filecard20(int drive)
{
    struct mfm_wd1002_geom entry;

    if (drive != 0 || MFMHD_DRIVES != 1)
        return 0;

    entry.cylinders = MFM_FILECARD20_CYLINDERS;
    entry.heads = MFM_FILECARD20_HEADS;
    entry.sectors = MFM_FILECARD20_SECTORS;
    entry.wp_cylinder = MFMHD_GEO_NONE;
    entry.rwc_cylinder = MFMHD_GEO_NONE;
    mfmhd_set_drive_geometry(drive, &entry, MFM_GEO_SRC_FILECARD20);

    printk("mfmhd: /dev/mfm%c using Seagate FileCard 20 split-unit geometry %u/%u/%u\n",
        'a' + (unsigned char)drive, MFM_FILECARD20_CYLINDERS,
        MFM_FILECARD20_HEADS, MFM_FILECARD20_SECTORS);
    return 1;
}

static void
mfmhd_recalibrate_unit(unsigned int port, int drive, unsigned int unit)
{
    mfmhd_build_command(mfmhd_cmdblk, MFM_CMD_RECALIBRATE, (unsigned char)unit,
        0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    (void)mfmhd_cmd(port, mfmhd_cmdblk, NULL, 0, NULL, 0);
}

static void
mfmhd_recalibrate_drive(unsigned int port, int drive)
{
    mfmhd_recalibrate_unit(port, drive, (unsigned int)drive & 1U);
}

static void
mfmhd_seek_unit(unsigned int port, int drive, unsigned int unit,
    unsigned short cylinder)
{
    mfmhd_build_command(mfmhd_cmdblk, MFM_CMD_SEEK, (unsigned char)unit,
        0, cylinder, 0, 0, mfmhd_drive_cmd_control(drive));
    (void)mfmhd_cmd(port, mfmhd_cmdblk, NULL, 0, NULL, 0);
}

static int
mfmhd_reset_controller(unsigned int port)
{
    unsigned int i;

    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
    mfmhd_debug_set(20, -1, port, 0);
    outb_p(0, port + MFM_HD_RESET);
    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);

    /* Give the adapter a short bus-turnaround delay before polling status. */
    for (i = 0; i < 256; i++)
        (void)STATUS(port);

    /*
     * A reset Xebec may enter command-out phase with SELECT still asserted.
     * Waiting for SELECT to clear deadlocks controllers in that state,
     * including MartyPC's model.  READY is the portable indication that the
     * adapter can accept the explicit select and command sequence below.
     *
     * Some controllers (the Amstrad PC1640's among them) never raise READY
     * after reset yet answer commands normally, so this wait is deliberately
     * short: the probe below is the real test and a long deadline here only
     * adds tens of seconds to every boot.
     */
    i = mfmhd_wait_status(port, MFM_STAT_READY, MFM_STAT_READY,
        mfmhd_phase_ticks(), "reset ready timeout");
    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
    mfmhd_debug_set(i ? 22 : 21, -1, port, i ? -1 : 0);
    return i;
}


static int INITPROC
mfmhd_probe_controller(unsigned int port, int drive)
{
    unsigned int attempt;
    unsigned int max_attempts;
    unsigned char st;

    max_attempts = (drive & 1) ? 1 : 3;
    mfmhd_debug_set(30, drive, port, 0);
    if (!(drive & 1))
        (void)mfmhd_reset_controller(port);

    for (attempt = 0; attempt < max_attempts; attempt++) {
        st = STATUS(port);
        if (st == 0xff) {
            if (!(drive & 1))
                printk("mfmhd: controller port=0x%x floating bus\n", port);
            mfmhd_debug_set(31, drive, port, -1);
            return -1;
        }

        mfmhd_build_command(mfmhd_cmdblk, MFM_CMD_TEST_DRIVE_READY,
            (unsigned char)drive, 0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
        mfmhd_quiet_probe = 1;
        if (!mfmhd_cmd(port, mfmhd_cmdblk, NULL, 0, NULL, 0)) {
            mfmhd_quiet_probe = 0;
            mfmhd_debug_set(32, drive, port, 0);
            return 0;
        }
        mfmhd_quiet_probe = 0;
        mfmhd_debug_set(33, drive, port, -1);
    }

    if (!(drive & 1)) {
        printk("mfmhd: /dev/mfm%c test-ready failed at port=0x%x; probing media anyway\n",
            'a' + (unsigned char)drive, port);
        return 0;
    }
    return -1;
}

static int INITPROC
mfmhd_vbr_has_fat_fs_type(const char *b)
{
    if (!memcmp(b + 0x36, "FAT12   ", 8))
        return 1;
    if (!memcmp(b + 0x36, "FAT16   ", 8))
        return 1;
    if (!memcmp(b + 0x36, "FAT32   ", 8))
        return 1;
    if (!memcmp(b + 0x52, "FAT32   ", 8))
        return 1;
    return 0;
}

static int INITPROC
mfmhd_vbr_bpb_plausible_fat(const char *b)
{
    unsigned int bps;
    unsigned int spc;
    unsigned int nfat;
    unsigned int root_ent;
    unsigned int tot16;
    sector_t tot32;
    unsigned int rsv;
    unsigned int fatlen;
    unsigned int media;

    bps = MFM_LE16(b, 11);
    spc = (unsigned int)(unsigned char)b[13];
    rsv = MFM_LE16(b, 14);
    nfat = (unsigned int)(unsigned char)b[16];
    root_ent = MFM_LE16(b, 17);
    tot16 = MFM_LE16(b, 19);
    media = (unsigned int)(unsigned char)b[21];
    fatlen = MFM_LE16(b, 22);
    tot32 = mfmhd_get_le_sector(b + 32);

    if (bps != MFM_SECTOR_BYTES)
        return 0;
    if (!spc || spc > 128)
        return 0;
    if (!rsv || rsv > 64)
        return 0;
    if (nfat < 1 || nfat > 2)
        return 0;
    if (media < 0xf0)
        return 0;
    if (!tot16 && !tot32)
        return 0;

    if (!memcmp(b + 0x52, "FAT32   ", 8) || !memcmp(b + 0x36, "FAT32   ", 8))
        return 1;

    if (!fatlen)
        return 0;
    if (!root_ent || (root_ent & 0x0f) != 0 || root_ent > 16384)
        return 0;
    return 1;
}

static int INITPROC
mfmhd_vbr_looks_like_fat_volume(const char *b)
{
    if (mfmhd_vbr_has_fat_fs_type(b))
        return 1;
    return mfmhd_vbr_bpb_plausible_fat(b);
}

static int INITPROC
mfmhd_has_mbr_signature(const char *b)
{
    return b[510] == (char)0x55 && b[511] == (char)0xaa;
}

static int INITPROC
mfmhd_apply_bpb_geometry(int drive, const char *secbuf)
{
    unsigned int sec;
    unsigned int heads;
    unsigned int cyl;
    unsigned int spcyl;
    unsigned int remainder;
    sector_t quotient;
    sector_t totalsz;
    struct mfm_wd1002_geom entry;

    sec = MFM_LE16(secbuf, 24);
    heads = MFM_LE16(secbuf, 26);
    /* FAT uses the 16-bit total first; the 32-bit field is its zero fallback. */
    totalsz = (sector_t)MFM_LE16(secbuf, 19);
    if (!totalsz)
        totalsz = mfmhd_get_le_sector(secbuf + 32);

    if (!sec || !heads || !totalsz)
        return 0;
    if (sec > 64 || heads > MFM_WD1002_MAX_HEADS)
        return 0;

    spcyl = mfmhd_sectors_per_cylinder(heads, sec);
    if (!spcyl)
        return 0;

    /*
     * Round the BPB sector count up to whole cylinders.  __divmod keeps the
     * 32-bit on-disk count at the ABI boundary and performs a 32-by-16 divide
     * using only 8086 word registers.  Saturate by rejecting any geometry
     * whose rounded cylinder count does not fit the controller's 16-bit CHS
     * field.
     */
    remainder = spcyl;
    quotient = __divmod(totalsz, &remainder);
    if (quotient > 0xffffUL)
        return 0;
    cyl = (unsigned int)quotient;
    if (remainder) {
        if (cyl == 0xffffU)
            return 0;
        cyl++;
    }

    if (!mfmhd_geometry_sane(cyl, heads, sec, totalsz))
        return 0;

    entry.cylinders = (unsigned short)cyl;
    entry.heads = (unsigned char)heads;
    entry.sectors = (unsigned char)sec;
    entry.wp_cylinder = MFMHD_GEO_NONE;
    entry.rwc_cylinder = MFMHD_GEO_NONE;
    mfmhd_set_drive_geometry(drive, &entry, MFM_GEO_SRC_BPB);

    printk("mfmhd: /dev/mfm%c FAT BPB geometry %u/%u/%u total=%lu\n",
        'a' + (unsigned char)drive, cyl, heads, sec, (unsigned long)totalsz);
    return 1;
}

static int INITPROC
mfmhd_apply_wd_sector0_geometry(int drive, const char *secbuf)
{
    unsigned int cyl;
    unsigned int heads;
    unsigned int rwc;
    unsigned int wp;
    struct mfm_wd1002_geom entry;

    /*
     * Linux xd gets WD1002 geometry from sector 0 before issuing the WD
     * set-parameters command.  Keep the same fallback for disks whose jumper
     * table does not match the actual low-level format.
     */
    cyl = MFM_LE16(secbuf, 0x1ad);
    heads = (unsigned int)(unsigned char)secbuf[0x1af];
    rwc = MFM_LE16(secbuf, 0x1b0);
    wp = MFM_LE16(secbuf, 0x1b4);

    if (!mfmhd_geometry_sane(cyl, heads, MFM_WD1002_SECTORS, 0))
        return 0;

    entry.cylinders = (unsigned short)cyl;
    entry.heads = (unsigned char)heads;
    entry.sectors = MFM_WD1002_SECTORS;
    entry.wp_cylinder = wp <= MFM_WD1002_MAX_CYLINDERS ?
        (unsigned short)wp : MFMHD_GEO_NONE;
    entry.rwc_cylinder = rwc <= MFM_WD1002_MAX_CYLINDERS ?
        (unsigned short)rwc : MFMHD_GEO_NONE;
    mfmhd_set_drive_geometry(drive, &entry, MFM_GEO_SRC_WD1002);

    printk("mfmhd: /dev/mfm%c WD sector0 geometry %u/%u/%u",
        'a' + (unsigned char)drive, drive_info[drive].cylinders,
        drive_info[drive].heads, drive_info[drive].sectors);
    mfmhd_print_cylinder_value("wp", drive_info[drive].wp_cylinder);
    mfmhd_print_cylinder_value("rwc", drive_info[drive].rwc_cylinder);
    printk("\n");
    return 1;
}

static int
mfmhd_rw_chunk(int drive, sector_t lba, unsigned int sectors, char *buffer,
    int write)
{
    unsigned int retry;
    unsigned int track_left;

    if (drive < 0 || drive >= MFM_MAX_DRIVES || !sectors)
        return -1;

    rw.heads = drive_info[drive].heads;
    rw.sec = drive_info[drive].sectors;
    if (!rw.heads || !rw.sec)
        return -1;

    rw.unit = (unsigned int)drive & 1U;
    rw.cmd_lba = lba;
    rw.max_cyl = drive_info[drive].cylinders;

    if (drive == 0 && drive_info[drive].source == MFM_GEO_SRC_FILECARD20) {
        if (lba >= MFM_FILECARD20_UNIT_SECTORS) {
            rw.unit = 1;
            rw.cmd_lba = lba - MFM_FILECARD20_UNIT_SECTORS;
        }

        rw.max_cyl = MFM_FILECARD20_UNIT_CYLINDERS;
        if (rw.cmd_lba >= MFM_FILECARD20_UNIT_SECTORS)
            return -1;
        rw.unit_left = MFM_FILECARD20_UNIT_SECTORS - rw.cmd_lba;
        if (!rw.unit_left)
            return -1;
        if ((sector_t)sectors > rw.unit_left)
            sectors = (unsigned int)rw.unit_left;
    }

    if (mfmhd_lba_to_chs(rw.cmd_lba, rw.heads, rw.sec, &rw.cyl, &rw.head, &rw.sector))
        return -1;

    if (rw.cyl >= rw.max_cyl || rw.head >= rw.heads || rw.sector >= rw.sec)
        return -1;

    track_left = rw.sec - rw.sector;
    if (sectors > track_left)
        sectors = track_left;
    if (sectors > MFMHD_MAX_BURST)
        sectors = MFMHD_MAX_BURST;
    if (sectors > MFM_DMA_SECTORS)
        sectors = MFM_DMA_SECTORS;
    if (drive_info[drive].force_single && sectors > 1)
        sectors = 1;

    for (retry = 0; retry < MFM_RETRIES; retry++) {
        if (retry) {
            if (mfmhd_probe_expired())
                break;              /* don't burn the boot on a bad unit */
            drive_info[drive].retry_count++;
            if (!mfmhd_slow_profile || retry == MFM_RETRIES - 1)
                printk("mfmhd: /dev/mfm%c retry %u %s lba=%lu unit=%u n=%u\n",
                    'a' + (unsigned char)drive, retry,
                    write ? "write" : "read", (unsigned long)lba, rw.unit,
                    sectors);
            mfmhd_recalibrate_unit(mfm_port, drive, rw.unit);
        }

        mfmhd_build_command(mfmhd_cmdblk,
            write ? MFM_CMD_WRITE_SECTORS : MFM_CMD_READ_SECTORS,
            (unsigned char)rw.unit, (unsigned char)rw.head, (unsigned short)rw.cyl,
            (unsigned char)rw.sector, (unsigned char)sectors,
            mfmhd_drive_cmd_control(drive));

        if (!mfmhd_cmd(mfm_port, mfmhd_cmdblk,
                write ? NULL : (unsigned char *)buffer,
                write ? 0 : sectors * MFM_SECTOR_BYTES,
                write ? (unsigned char *)buffer : NULL,
                write ? sectors * MFM_SECTOR_BYTES : 0)) {
            if (drive_info[drive].force_single && sectors == 1) {
                if (drive_info[drive].single_ok_streak < 255)
                    drive_info[drive].single_ok_streak++;
                if (drive_info[drive].single_ok_streak >= MFM_SINGLE_RECOVER_OK) {
                    drive_info[drive].force_single = 0;
                    drive_info[drive].single_ok_streak = 0;
                    printk("mfmhd: /dev/mfm%c restoring burst transfers after stable single-sector run\n",
                        'a' + (unsigned char)drive);
                }
            }
            return (int)sectors;
        }
    }

    if (!drive_info[drive].force_single) {
        drive_info[drive].force_single = 1;
        drive_info[drive].single_ok_streak = 0;
        printk("mfmhd: /dev/mfm%c enabling single-sector transfer mode after %s errors\n",
            'a' + (unsigned char)drive, write ? "write" : "read");
    } else {
        drive_info[drive].single_ok_streak = 0;
    }

    printk("mfmhd: /dev/mfm%c %s failed lba=%lu unit=%u chs=%u/%u/%u n=%u\n",
        'a' + (unsigned char)drive, write ? "write" : "read",
        (unsigned long)lba, rw.unit, rw.cyl, rw.head, rw.sector, sectors);
    return -1;
}

static int INITPROC
mfmhd_probe_drive(int drive)
{
    unsigned int tries;
    char *secbuf;
    int read_ok;
    int read_ok2;
    int fat_vbr;
    char *secbuf2;

    mfmhd_debug_set(50, drive, mfm_port, 0);
    if (mfmhd_probe_controller(mfm_port, drive)) {
        mfmhd_debug_set(51, drive, mfm_port, -1);
        return -1;
    }

    if (!mfmhd_set_geometry_from_user(drive)) {
        if (mfmhd_controller_is_seagate()) {
            if (!mfmhd_set_geometry_from_seagate(drive, mfm_port) &&
                    !mfmhd_set_geometry_from_filecard20(drive)) {
                printk("mfmhd: /dev/mfm%c has no Seagate geometry\n",
                    'a' + (unsigned char)drive);
            }
        } else {
            mfmhd_set_wd1002_geometry_by_jumper(drive, mfm_port);
        }
    }

    if (!mfmhd_geometry_sane(drive_info[drive].cylinders,
            drive_info[drive].heads, drive_info[drive].sectors, 0)) {
        printk("mfmhd: /dev/mfm%c has unusable geometry\n",
            'a' + (unsigned char)drive);
        mfmhd_debug_set(52, drive, mfm_port, -1);
        return -1;
    }

    /* Read sector 0 before any WD sector-0 geometry fallback. */
    mfmhd_recalibrate_drive(mfm_port, drive);

    secbuf = (char *)heap_alloc(MFM_SECTOR_BYTES, HEAP_TAG_DRVR);
    if (!secbuf) {
        printk("mfmhd: unable to allocate probe buffer\n");
        return -1;
    }
    secbuf2 = (char *)heap_alloc(MFM_SECTOR_BYTES, HEAP_TAG_DRVR);
    if (!secbuf2) {
        printk("mfmhd: unable to allocate second probe buffer\n");
        heap_free(secbuf);
        return -1;
    }

    read_ok = 0;
    for (tries = 0; tries < MFM_MBR_READ_TRIES; tries++) {
        if (mfmhd_rw_chunk(drive, 0, 1, secbuf, 0) == 1) {
            read_ok = 1;
            break;
        }
        if (mfmhd_probe_expired())
            break;
        mfmhd_recalibrate_drive(mfm_port, drive);
    }

    if (!read_ok) {
        printk("mfmhd: /dev/mfm%c unable to read sector 0\n",
            'a' + (unsigned char)drive);
        mfmhd_debug_set(54, drive, mfm_port, -1);
        heap_free(secbuf2);
        heap_free(secbuf);
        return -1;
    }

    /*
     * Probe-time stability check: read LBA0 twice and compare.
     * If unstable, start in single-sector mode immediately.
     */
    read_ok2 = (mfmhd_rw_chunk(drive, 0, 1, secbuf2, 0) == 1);
    if (read_ok2 && memcmp(secbuf, secbuf2, MFM_SECTOR_BYTES) != 0) {
        drive_info[drive].force_single = 1;
        drive_info[drive].single_ok_streak = 0;
        printk("mfmhd: /dev/mfm%c probe instability detected (LBA0 mismatch), forcing single-sector mode\n",
            'a' + (unsigned char)drive);
    }

    fat_vbr = mfmhd_vbr_looks_like_fat_volume(secbuf);

    if (drive_info[drive].source == MFM_GEO_SRC_USER) {
        printk("mfmhd: /dev/mfm%c preserving explicit user geometry\n",
            'a' + (unsigned char)drive);
    } else if (fat_vbr) {
        if (!mfmhd_apply_bpb_geometry(drive, secbuf))
            printk("mfmhd: /dev/mfm%c FAT BPB geometry not usable; keeping %s table\n",
                'a' + (unsigned char)drive,
                mfmhd_source_name(drive_info[drive].source));
    } else if (!mfmhd_controller_is_seagate() &&
            mfmhd_apply_wd_sector0_geometry(drive, secbuf)) {
    } else if (mfmhd_has_mbr_signature(secbuf)) {
        printk("mfmhd: /dev/mfm%c boot signature detected\n",
            'a' + (unsigned char)drive);
    } else {
        printk("mfmhd: /dev/mfm%c sector 0 has no DOS boot signature; raw disk attached\n",
            'a' + (unsigned char)drive);
    }

    if (!mfmhd_controller_is_seagate()) {
        if (mfmhd_set_drive_parameters(mfm_port, drive)) {
            printk("mfmhd: /dev/mfm%c unable to program WD1002 parameters\n",
                'a' + (unsigned char)drive);
            mfmhd_debug_set(56, drive, mfm_port, -1);
            heap_free(secbuf2);
            heap_free(secbuf);
            return -1;
        }
        printk("mfmhd: /dev/mfm%c programmed WD1002 parameters %u/%u/%u\n",
            'a' + (unsigned char)drive, drive_info[drive].cylinders,
            drive_info[drive].heads, drive_info[drive].sectors);
        mfmhd_recalibrate_drive(mfm_port, drive);
    }

    heap_free(secbuf2);
    heap_free(secbuf);
    mfmhd_debug_set(55, drive, mfm_port, 0);
    return 0;
}

struct gendisk * INITPROC mfmhd_init(void)
{
    int drive;
    int i;
    int hdcnt;
    int hdmax;
    sector_t sectors;

    /* mfm=off in /bootopts leaves the port at -1 */
    if (mfm_port < 0 || dev_disabled(DEV_MFMA)) {
        printk("mfmhd: disabled\n");
        return NULL;
    }

    mfmhd_slow_profile = mfm_flags & MFMF_SLOW;
    mfmhd_pio = mfm_flags & MFMF_PIO;
    mfmhd_trace = mfm_flags & MFMF_TRACE;
    mfmhd_irq_mode = mfm_flags & MFMF_IRQ;
    mfmhd_extwrite = mfm_flags & MFMF_EXTWRITE;

    /* Set once here, before any transfer is programmed. */
    if (mfmhd_extwrite) {
        outb_p(DMA1_CMD_EXTWRITE, DMA1_CMD_REG);
        printk("mfmhd: 8237 extended write enabled\n");
    }

    mfmhd_init_ports();
    mfmhd_debug_set(10, -1, mfm_port, 0);
    memset(&mfmhd_part[0], 0xff, sizeof(mfmhd_part));
    for (i = 0; i < (MFM_MAX_DRIVES << MFM_MINOR_SHIFT); i++) {
        mfmhd_part[i].start_sect = NOPART;
        mfmhd_part[i].nr_sects = 0;
    }

    hdcnt = 0;
    hdmax = -1;
    mfmhd_probe_deadline = jiffies() + MFM_PROBE_TICKS;
    for (drive = 0; drive < MFMHD_DRIVES; drive++) {
        if (mfmhd_probe_expired()) {
            printk("mfmhd: probe deadline reached, skipping /dev/mfm%c\n",
                'a' + (unsigned char)drive);
            continue;
        }
        mfmhd_init_drive_state(drive);
        if (mfmhd_probe_drive(drive))
            continue;

        sectors = mfmhd_chs_capacity(drive_info[drive].cylinders,
            drive_info[drive].heads, drive_info[drive].sectors);

        mfmhd_part[drive << MFM_MINOR_SHIFT].start_sect = 0;
        mfmhd_part[drive << MFM_MINOR_SHIFT].nr_sects = sectors;
        mfmhd_drive_info[drive].cylinders = drive_info[drive].cylinders;
        mfmhd_drive_info[drive].heads = (int)drive_info[drive].heads;
        mfmhd_drive_info[drive].sectors = (int)drive_info[drive].sectors;
        mfmhd_drive_info[drive].sector_size = MFM_SECTOR_BYTES;
        mfmhd_drive_info[drive].fdtype = HARDDISK;
        hdcnt++;
        if (drive > hdmax)
            hdmax = drive;

        printk("mfmhd: /dev/mfm%c source=%s geometry %u/%u/%u sectors=%lu\n",
            'a' + (unsigned char)drive,
            mfmhd_source_name(drive_info[drive].source),
            drive_info[drive].cylinders, drive_info[drive].heads,
            drive_info[drive].sectors, (unsigned long)sectors);
    }

    /* Probing is over: restore the full retry budget for normal I/O. */
    mfmhd_probe_deadline = 0;

    if (!hdcnt) {
        printk("mfmhd: no XT MFM drives found\n");
        mfmhd_debug_set(90, -1, mfm_port, -1);
        return NULL;
    }

    mfmhd_bounce = (char *)heap_alloc(MFM_BOUNCE_BYTES, HEAP_TAG_DRVR);
    if (!mfmhd_bounce) {
        printk("mfmhd: unable to allocate I/O bounce buffer\n");
        return NULL;
    }

    mfmhd_gendisk.nr_hd = hdmax + 1;

    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &mfmhd_fops)) {
        heap_free(mfmhd_bounce);
        mfmhd_bounce = NULL;
        return NULL;
    }
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

    /* The block layer scans partitions after this request path is live. */
    mfmhd_initialized = 1;

    if (mfmhd_irq_mode) {
        if (request_irq(mfm_irq, mfmhd_interrupt, INT_GENERIC))
            printk("mfmhd: irq %d busy, staying polled\n", mfm_irq);
        else
            mfmhd_irq_armed = 1;
    }

    printk("mfmhd: found %d hard drive%c at port 0x%x irq %d, %s %s%s\n",
        hdcnt, hdcnt == 1 ? ' ' : 's', mfm_port, mfm_irq,
        mfmhd_irq_armed ? "interrupt" : "polled",
        mfmhd_pio ? "pio" : "dma=3", mfmhd_extwrite ? " xw" : "");
    mfmhd_debug_set(91, -1, mfm_port, 0);
    return &mfmhd_gendisk;
}

/*
 * Park heads before the machine stops.  The last cylinder is the landing
 * zone on an ST-506 drive, which is where an unpowered head should settle.
 * This goes out as the controller's own SEEK, so it works with the option
 * ROM strapped off, which is the case CONFIG_BLK_DEV_MFMHD exists for, and
 * leaves the driver free of BIOS calls for the protected mode kernel.
 * Errors are ignored, there being nothing left to retry against.
 */
void mfmhd_park_all(void)
{
    int drive;

    if (!mfmhd_initialized)
        return;

    for (drive = 0; drive < MFMHD_DRIVES; drive++) {
        if (!drive_info[drive].heads || !drive_info[drive].cylinders)
            continue;

        /*
         * A FileCard 20 is one drive spread over both controller units,
         * each with an actuator of its own, so park the pair.
         */
        if (drive == 0 && drive_info[drive].source == MFM_GEO_SRC_FILECARD20) {
            mfmhd_seek_unit(mfm_port, drive, 0,
                MFM_FILECARD20_UNIT_CYLINDERS - 1);
            mfmhd_seek_unit(mfm_port, drive, 1,
                MFM_FILECARD20_UNIT_CYLINDERS - 1);
            continue;
        }

        mfmhd_seek_unit(mfm_port, drive, (unsigned int)drive & 1U,
            drive_info[drive].cylinders - 1);    /* cylinders are zero based */
    }
}

static int
mfmhd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
    unsigned int arg)
{
    int drive;

    if (cmd == IOCTL_BLK_GET_SECTOR_SIZE)
        return MFM_SECTOR_BYTES;

    if (!inode)
        return -EINVAL;

    drive = DEVICE_NR(inode->i_rdev);
    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return -ENODEV;
    if (!mfmhd_initialized || !drive_info[drive].heads ||
            !drive_info[drive].sectors)
        return -ENXIO;

    switch (cmd) {
    case HDIO_GETGEO:
        return ioctl_hdio_geometry(&mfmhd_gendisk, inode->i_rdev,
            (struct hd_geometry *)arg);
    }
    return -EINVAL;
}

static int
mfmhd_open(struct inode *inode, struct file *filp)
{
    unsigned int minor;
    int target;

    if (!inode)
        return -EINVAL;

    minor = MINOR(inode->i_rdev);
    target = DEVICE_NR(inode->i_rdev);

    if (target < 0 || target >= MFM_MAX_DRIVES || !mfmhd_initialized)
        return -ENXIO;
    if (minor >= (MFM_MAX_DRIVES << MFM_MINOR_SHIFT))
        return -ENXIO;
    if (mfmhd_part[minor].start_sect == NOPART ||
        mfmhd_part[minor].nr_sects == 0)
        return -ENXIO;

    access_count[target]++;
    if (mfmhd_part[minor].nr_sects >= 0x00400000L)
        inode->i_size = 0x7fffffffL;
    else
        inode->i_size = mfmhd_part[minor].nr_sects << 9;

    return 0;
}

static void
mfmhd_release(struct inode *inode, struct file *filp)
{
    kdev_t dev;
    int target;

    if (!inode)
        return;
    dev = inode->i_rdev;
    target = DEVICE_NR(dev);
    if (target < 0 || target >= MFM_MAX_DRIVES || access_count[target] <= 0)
        return;

    if (--access_count[target] == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
}

void do_mfmhd_request(void)
{
    sector_t start;
    sector_t part_size;
    unsigned int count;
    unsigned int left;
    unsigned int pass;
    char *buff;
    char *iobuf;
    int minor;
    int drive;
    int direct;
    int xfer;

    while (1) {
        struct request *req = CURRENT;
        if (!req)
            return;
        CHECK_REQUEST(req);

        if (mfmhd_initialized != 1) {
            end_request(0);
            continue;
        }

        minor = MINOR(req->rq_dev);
        drive = minor >> MFM_MINOR_SHIFT;
        if (drive < 0 || drive >= MFM_MAX_DRIVES ||
                minor >= (MFM_MAX_DRIVES << MFM_MINOR_SHIFT) ||
                mfmhd_part[minor].start_sect == NOPART ||
                mfmhd_part[minor].nr_sects == 0) {
            printk("mfmhd: non-existent drive %D\n", req->rq_dev);
            end_request(0);
            continue;
        }

        count = req->rq_nr_sectors;
        part_size = mfmhd_part[minor].nr_sects;
        if (!count || req->rq_sector >= part_size ||
                (sector_t)count > part_size - req->rq_sector) {
            printk("mfmhd: out-of-range request dev=%D sector=%lu count=%u\n",
                req->rq_dev, (unsigned long)req->rq_sector,
                count);
            end_request(0);
            continue;
        }

        start = req->rq_sector + mfmhd_part[minor].start_sect;
        buff = req->rq_buffer;
        direct = (req->rq_seg == kernel_ds);
        left = count;

        if (mfmhd_trace)
            printk("mfmhd: req %s min=%u lba=%lu n=%u %s\n",
                req->rq_cmd == WRITE ? "wr" : "rd", minor,
                (unsigned long)start, count, direct ? "direct" : "bounce");

        while (left > 0) {
            pass = left;
            if (pass > MFMHD_MAX_BURST)
                pass = MFMHD_MAX_BURST;
            if (!direct && pass > MFM_BOUNCE_SECTORS)
                pass = MFM_BOUNCE_SECTORS;
            iobuf = direct ? buff : mfmhd_bounce;

            if (req->rq_cmd == WRITE && !direct)
                xms_fmemcpyw(mfmhd_bounce, kernel_ds, buff, req->rq_seg,
                    (unsigned int)(pass * (MFM_SECTOR_BYTES >> 1)));

            xfer = mfmhd_rw_chunk(drive, start, (unsigned int)pass,
                iobuf, req->rq_cmd == WRITE);
            /*
             * A zero-sector transfer cannot make progress: "left -= 0" would
             * spin this loop forever without printing anything.  Treat it as
             * an I/O error rather than hanging the request silently.
             */
            if (xfer <= 0) {
                drive_info[drive].io_error_count++;
                printk("mfmhd: io error drive=%d start=%lu cmd=%s xfer=%d\n", drive,
                    (unsigned long)start, req->rq_cmd == WRITE ? "write" : "read",
                    xfer);
                end_request(0);
                break;
            }

            if (req->rq_cmd == READ && !direct)
                xms_fmemcpyw(buff, req->rq_seg, mfmhd_bounce, kernel_ds,
                    (unsigned int)(xfer * (MFM_SECTOR_BYTES >> 1)));

            left -= (unsigned int)xfer;
            start += (sector_t)xfer;
            buff += xfer * MFM_SECTOR_BYTES;
        }

        if (!left) {
            /*
             * Marks the driver handing the request back.  If a freeze happens
             * after this line prints, the lock-up is above the driver (buffer
             * cache / filesystem), not in the phase engine.
             */
            if (mfmhd_trace)
                printk("mfmhd: req ok\n");
            end_request(1);
        }
    }
}
