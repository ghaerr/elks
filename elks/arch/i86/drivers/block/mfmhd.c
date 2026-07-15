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

#include "mfmhd.h"

#define MAJOR_NR        MFMHD_MAJOR
#define ATDISK
#include "blk.h"

/* WD1002A-WX1 register offsets from the selected I/O base. */
#define MFM_HD_DATA             0
#define MFM_HD_STATUS           1
#define MFM_HD_RESET            1
#define MFM_HD_SELECT           2
#define MFM_HD_JUMPER           2
#define MFM_HD_CONTROL          3

/* Hardware status register bits, mirrored in mfmhd.h for user tools. */
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
#define MFM_CMD_FORMAT_DRIVE            MFMHD_CMD_FORMAT_DRIVE
#define MFM_CMD_VERIFY_SECTORS          MFMHD_CMD_VERIFY_SECTORS
#define MFM_CMD_FORMAT_TRACK            MFMHD_CMD_FORMAT_TRACK
#define MFM_CMD_FORMAT_BAD_TRACK        MFMHD_CMD_FORMAT_BAD_TRACK
#define MFM_CMD_READ_SECTORS            MFMHD_CMD_READ_SECTORS
#define MFM_CMD_WRITE_SECTORS           MFMHD_CMD_WRITE_SECTORS
#define MFM_CMD_SEEK                    MFMHD_CMD_SEEK
#define MFM_CMD_INIT_DRIVE_PARAMETERS   MFMHD_CMD_INIT_DRIVE_PARAMETERS
#define MFM_CMD_READ_ECC_BURST          MFMHD_CMD_READ_ECC_BURST
#define MFM_CMD_READ_SECTOR_BUFFER      MFMHD_CMD_READ_SECTOR_BUFFER
#define MFM_CMD_WRITE_SECTOR_BUFFER     MFMHD_CMD_WRITE_SECTOR_BUFFER
#define MFM_CMD_BUFFER_DIAGNOSTIC       MFMHD_CMD_BUFFER_DIAGNOSTIC
#define MFM_CMD_DRIVE_DIAGNOSTIC        MFMHD_CMD_DRIVE_DIAGNOSTIC
#define MFM_CMD_CONTROLLER_DIAGNOSTIC   MFMHD_CMD_CONTROLLER_DIAGNOSTIC
#define MFM_CMD_READ_LONG               MFMHD_CMD_READ_LONG
#define MFM_CMD_WRITE_LONG              MFMHD_CMD_WRITE_LONG
#define MFM_CMD_ST11_GET_GEOMETRY       MFMHD_CMD_ST11_GET_GEOMETRY

/* Command block byte 5: R1/R2/0/0/0/SP2/SP1/SP0. */
#define MFM_CCB_R1_NO_RETRY         MFMHD_CCB_R1_NO_RETRY
#define MFM_CCB_R2_IMMEDIATE_ECC    MFMHD_CCB_R2_IMMEDIATE_ECC
#define MFM_CCB_STEP_MASK           MFMHD_CCB_STEP_MASK
#define MFM_CCB_RESERVED_MASK       MFMHD_CCB_RESERVED_MASK

#define MFM_COMMAND_BYTES       6
#define MFM_SETPARAM_PARAMETER_BYTES 8
#define MFM_TRACE_COMMAND_BYTES \
    (MFM_COMMAND_BYTES + MFM_SETPARAM_PARAMETER_BYTES)
#define MFM_SENSE_BYTES         4
#define MFM_SECTOR_BYTES        512
#define MFM_BOUNCE_SECTORS     1
/*
 * Normal requests use the first 512 bytes.  Maintenance READ LONG and
 * WRITE LONG commands transfer four additional ECC bytes.  Reserving those
 * bytes here avoids ever placing a sector-sized object on the tiny ELKS
 * kernel stack.
 */
#define MFM_BOUNCE_BYTES       (MFM_SECTOR_BYTES + 4)

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
#error "XT MFM driver requires at least 516 bytes of low DMA memory"
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

#define MFM_MAX_CONTROLLERS     2
#define MFM_UNITS_PER_CTLR      2
#define MFM_MAX_DRIVES          (MFM_MAX_CONTROLLERS * MFM_UNITS_PER_CTLR)
#define MFM_MINOR_SHIFT         3
#define MFM_MAX_PARTITIONS      (1 << MFM_MINOR_SHIFT)

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
#define MFM_READY_TICKS         (HZ / 10)
#define MFM_PHASE_TICKS         (HZ * 2)
#define MFM_DISK_TICKS          (HZ * 12)
#define MFM_DIAG_TICKS          (HZ * 20)
#define MFM_SELECT_TICKS        (HZ * 2)
#define MFM_SLOW_PHASE_TICKS    (HZ * 4)
#define MFM_SLOW_DISK_TICKS     (HZ * 24)
#define MFM_SLOW_DIAG_TICKS     (HZ * 40)
#define MFM_SLOW_SELECT_TICKS   (HZ * 4)
/*
 * Formatting is an optional maintenance operation.  A 10-minute ceiling is
 * long enough for XT media while keeping the timeout constant within a
 * 16-bit unsigned word.  The jiffies interface itself is the existing ELKS
 * two-word clock ABI; no wide multiply is emitted here.
 */
#define MFM_FORMAT_TICKS        (600U * HZ)
#define MFM_SINGLE_RECOVER_OK   64
#define MFM_DEFAULT_ECC_LENGTH  11

#define MFMHD_GEO_NONE          0xffff

/*
 * WD1002A-WX1 I/O bases from W4:
 *   320h: W4 pins 2-3 closed
 *   324h: W4 pins 1-2 closed
 * The controller is polled here; IRQ values are logged only for maintenance.
 */
#ifndef MFM_HD1_PORT
# ifdef CONFIG_MFMHD_PRIMARY_PORT
#  define MFM_HD1_PORT CONFIG_MFMHD_PRIMARY_PORT
# elif defined(MFM_HD1_DEFAULT_PORT)
#  define MFM_HD1_PORT MFM_HD1_DEFAULT_PORT
# else
#  define MFM_HD1_PORT 0x320
# endif
#endif

#ifndef MFM_HD2_PORT
# ifdef CONFIG_MFMHD_SECONDARY_PORT
#  define MFM_HD2_PORT CONFIG_MFMHD_SECONDARY_PORT
# else
#  define MFM_HD2_PORT          0x324
# endif
#endif

#ifndef MFM_HD1_IRQ
#define MFM_HD1_IRQ             5
#endif

#ifndef MFM_HD2_IRQ
#define MFM_HD2_IRQ             2
#endif

#ifndef MFM_HD_PRIMARY_PORT
#define MFM_HD_PRIMARY_PORT     MFM_HD1_PORT
#endif

#ifndef MFM_HD_SECONDARY_PORT
#define MFM_HD_SECONDARY_PORT   MFM_HD2_PORT
#endif

#ifndef MFM_HD_PORT_SWAP
#define MFM_HD_PORT_SWAP        0
#endif

/* Raw jumper port polarity.  Most ISA jumper inputs read 1 when open. */
#ifndef MFMHD_JUMPER_OPEN_IS_ONE
#define MFMHD_JUMPER_OPEN_IS_ONE    1
#endif

/* Choose the WD1002A-WX1 BIOS switch table used for jumper geometry. */
#define MFMHD_BIOS_SUPER        0
#define MFMHD_BIOS_G            1
#define MFMHD_BIOS_H            2

#define MFMHD_CTRL_WD1002       0
#define MFMHD_CTRL_SEAGATE      1
#if defined(CONFIG_MFMHD_CTRL_SEAGATE)
#define MFMHD_CONTROLLER        MFMHD_CTRL_SEAGATE
#else
#define MFMHD_CONTROLLER        MFMHD_CTRL_WD1002
#endif

#if MFMHD_CONTROLLER == MFMHD_CTRL_SEAGATE
#define MFMHD_MAX_BURST         MFM_ST11_MAX_BURST
#else
#define MFMHD_MAX_BURST         MFM_WD1002_MAX_BURST
#endif

#if defined(CONFIG_MFMHD_BIOS_G)
#define MFMHD_WD1002_BIOS       MFMHD_BIOS_G
#elif defined(CONFIG_MFMHD_BIOS_H)
#define MFMHD_WD1002_BIOS       MFMHD_BIOS_H
#elif defined(CONFIG_MFMHD_BIOS_SUPER)
#define MFMHD_WD1002_BIOS       MFMHD_BIOS_SUPER
#endif
#ifndef MFMHD_WD1002_BIOS
#define MFMHD_WD1002_BIOS       MFMHD_BIOS_SUPER
#endif

/* Optional fixed per-drive geometry overrides: cylinders, heads, sectors. */
#ifndef MFMHD_DRIVE0_CYLINDERS
# ifdef CONFIG_MFMHD_DRIVE0_CYLINDERS
#  define MFMHD_DRIVE0_CYLINDERS CONFIG_MFMHD_DRIVE0_CYLINDERS
# else
#  define MFMHD_DRIVE0_CYLINDERS 0
# endif
#endif
#ifndef MFMHD_DRIVE0_HEADS
# ifdef CONFIG_MFMHD_DRIVE0_HEADS
#  define MFMHD_DRIVE0_HEADS CONFIG_MFMHD_DRIVE0_HEADS
# else
#  define MFMHD_DRIVE0_HEADS 0
# endif
#endif
#ifndef MFMHD_DRIVE0_SECTORS
# ifdef CONFIG_MFMHD_DRIVE0_SECTORS
#  define MFMHD_DRIVE0_SECTORS CONFIG_MFMHD_DRIVE0_SECTORS
# else
#  define MFMHD_DRIVE0_SECTORS 0
# endif
#endif
#ifndef MFMHD_DRIVE1_CYLINDERS
# ifdef CONFIG_MFMHD_DRIVE1_CYLINDERS
#  define MFMHD_DRIVE1_CYLINDERS CONFIG_MFMHD_DRIVE1_CYLINDERS
# else
#  define MFMHD_DRIVE1_CYLINDERS 0
# endif
#endif
#ifndef MFMHD_DRIVE1_HEADS
# ifdef CONFIG_MFMHD_DRIVE1_HEADS
#  define MFMHD_DRIVE1_HEADS CONFIG_MFMHD_DRIVE1_HEADS
# else
#  define MFMHD_DRIVE1_HEADS 0
# endif
#endif
#ifndef MFMHD_DRIVE1_SECTORS
# ifdef CONFIG_MFMHD_DRIVE1_SECTORS
#  define MFMHD_DRIVE1_SECTORS CONFIG_MFMHD_DRIVE1_SECTORS
# else
#  define MFMHD_DRIVE1_SECTORS 0
# endif
#endif
#ifndef MFMHD_DRIVE2_CYLINDERS
# ifdef CONFIG_MFMHD_DRIVE2_CYLINDERS
#  define MFMHD_DRIVE2_CYLINDERS CONFIG_MFMHD_DRIVE2_CYLINDERS
# else
#  define MFMHD_DRIVE2_CYLINDERS 0
# endif
#endif
#ifndef MFMHD_DRIVE2_HEADS
# ifdef CONFIG_MFMHD_DRIVE2_HEADS
#  define MFMHD_DRIVE2_HEADS CONFIG_MFMHD_DRIVE2_HEADS
# else
#  define MFMHD_DRIVE2_HEADS 0
# endif
#endif
#ifndef MFMHD_DRIVE2_SECTORS
# ifdef CONFIG_MFMHD_DRIVE2_SECTORS
#  define MFMHD_DRIVE2_SECTORS CONFIG_MFMHD_DRIVE2_SECTORS
# else
#  define MFMHD_DRIVE2_SECTORS 0
# endif
#endif
#ifndef MFMHD_DRIVE3_CYLINDERS
# ifdef CONFIG_MFMHD_DRIVE3_CYLINDERS
#  define MFMHD_DRIVE3_CYLINDERS CONFIG_MFMHD_DRIVE3_CYLINDERS
# else
#  define MFMHD_DRIVE3_CYLINDERS 0
# endif
#endif
#ifndef MFMHD_DRIVE3_HEADS
# ifdef CONFIG_MFMHD_DRIVE3_HEADS
#  define MFMHD_DRIVE3_HEADS CONFIG_MFMHD_DRIVE3_HEADS
# else
#  define MFMHD_DRIVE3_HEADS 0
# endif
#endif
#ifndef MFMHD_DRIVE3_SECTORS
# ifdef CONFIG_MFMHD_DRIVE3_SECTORS
#  define MFMHD_DRIVE3_SECTORS CONFIG_MFMHD_DRIVE3_SECTORS
# else
#  define MFMHD_DRIVE3_SECTORS 0
# endif
#endif

/* Number of MFM units to probe/register (a..). */
#ifndef MFMHD_ACTIVE_DRIVES
# ifdef CONFIG_MFMHD_ACTIVE_DRIVES
#  define MFMHD_ACTIVE_DRIVES CONFIG_MFMHD_ACTIVE_DRIVES
# else
#  define MFMHD_ACTIVE_DRIVES   2
# endif
#endif
#if MFMHD_ACTIVE_DRIVES < 1 || MFMHD_ACTIVE_DRIVES > MFM_MAX_DRIVES
#error CONFIG_MFMHD_ACTIVE_DRIVES must be between 1 and 4
#endif

/* Compile-time diagnostics are quiet by default for production builds. */
#ifndef MFMHD_TRACE
# ifdef CONFIG_MFMHD_TRACE
#  define MFMHD_TRACE           1
# else
#  define MFMHD_TRACE           0
# endif
#endif
#ifndef MFMHD_TRACE_BUDGET
#define MFMHD_TRACE_BUDGET      128
#endif

#ifndef CONFIG_MFMHD_STEP_CODE
#define CONFIG_MFMHD_STEP_CODE  MFMHD_STEP_3MS_PREFERRED
#endif
#ifndef CONFIG_MFMHD_NO_RETRY
#define CONFIG_MFMHD_NO_RETRY   0
#endif
#ifndef CONFIG_MFMHD_ECC_IMMEDIATE
#define CONFIG_MFMHD_ECC_IMMEDIATE 0
#endif
#ifndef CONFIG_MFMHD_UNSAFE_CTRL_IOCTLS
#define CONFIG_MFMHD_UNSAFE_CTRL_IOCTLS 0
#endif
#ifndef CONFIG_MFMHD_SLOW
#define CONFIG_MFMHD_SLOW       0
#endif

#define STATUS(port)            inb_p((port) + MFM_HD_STATUS)
#define DATA(port)              inb_p((port) + MFM_HD_DATA)
#define DATA_OUT(port, val)     outb_p((val), (port) + MFM_HD_DATA)

#define MFM_LE16(b, o) \
    ((unsigned int)(unsigned char)(b)[(o)] | \
    ((unsigned int)(unsigned char)(b)[(o) + 1] << 8))

static sector_t
mfmhd_get_le_sector(const char *bytes)
{
    union mfm_sector_words result;

    result.word.low = MFM_LE16(bytes, 0);
    result.word.high = MFM_LE16(bytes, 2);
    return result.value;
}

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
    unsigned char type_index;
    unsigned char source;
    unsigned char force_single;
    unsigned char single_ok_streak;
    unsigned char cmd_control;
    unsigned char last_csb;
    unsigned char last_sense[MFM_SENSE_BYTES];
    unsigned int retry_count;
    unsigned int cmd_error_count;
    unsigned int io_error_count;
    unsigned int single_enable_count;
    unsigned int single_restore_count;
    unsigned int probe_mismatch_count;
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
static void mfmhd_init_ports(void);
static unsigned int mfmhd_select_port(int drive);
static int mfmhd_probe_drive(int drive);
static void mfmhd_recalibrate_drive(unsigned int port, int drive);
static unsigned char mfmhd_default_cmd_control(void);
static unsigned char mfmhd_sanitize_cmd_control(unsigned char cmd_control);
static unsigned char mfmhd_drive_cmd_control(int drive);
static int mfmhd_controller_is_seagate(void);
static void mfmhd_build_command(unsigned char *cmdblk, unsigned char op,
    unsigned char drive, unsigned char head, unsigned short cylinder,
    unsigned char sector, unsigned char count, unsigned char cmd_control);
static void mfmhd_build_setparam_command(unsigned char *cmdblk,
    unsigned char *params, unsigned char drive, unsigned char heads,
    unsigned short cylinders, unsigned short rwc_cylinder,
    unsigned short wp_cylinder, unsigned char ecc,
    unsigned char cmd_control);
static int mfmhd_cmd_len(unsigned int port, unsigned char *cmdblk,
    unsigned int cmdlen, unsigned char *read, unsigned int rlen,
    unsigned char *write, unsigned int wlen, unsigned char *sense);
static int mfmhd_cmd(unsigned int port, unsigned char *cmdblk,
    unsigned char *read, unsigned int rlen, unsigned char *write,
    unsigned int wlen, unsigned char *sense);
static int mfmhd_set_drive_parameters(unsigned int port, int drive);
static int mfmhd_set_geometry_from_seagate(int drive, unsigned int port);
static int mfmhd_set_geometry_from_filecard20(int drive);
static int mfmhd_rw_chunk(int drive, sector_t lba, unsigned int sectors,
    char *buffer, int write);
static void mfmhd_write_control(unsigned int port, unsigned char value);
static void mfmhd_dma_stop(void);
static int mfmhd_reset_controller(unsigned int port);
#ifdef CONFIG_MFMHD_DIAG_IOCTLS
static int mfmhd_request_sense(unsigned int port, int drive,
    unsigned char *sense, unsigned char *csb);
#endif
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
static unsigned int io_ports[MFM_MAX_CONTROLLERS] = {
    MFM_HD_PRIMARY_PORT, MFM_HD_SECONDARY_PORT
};
static int mfmhd_irq_map[MFM_MAX_CONTROLLERS] = { MFM_HD1_IRQ, MFM_HD2_IRQ };
static unsigned char mfmhd_control_shadow[MFM_MAX_CONTROLLERS];
static int mfmhd_quiet_probe;
static char *mfmhd_bounce;
int mfmhd_slow_profile = CONFIG_MFMHD_SLOW;

#if MFMHD_TRACE
static unsigned int mfmhd_trace_budget = MFMHD_TRACE_BUDGET;
static unsigned int mfmhd_trace_seq;

static void
mfmhd_debug_set(int stage, int drive, unsigned int port, int error)
{
    unsigned char st;

    st = port ? STATUS(port) : 0xff;
    printk("mfmhd-debug: stage=%d drive=%d port=0x%x status=%02x error=%d\n",
        stage, drive, port, st, error);
}
#else
#define mfmhd_debug_set(stage, drive, port, error) ((void)0)
#endif

static unsigned int
mfmhd_controller_from_port(unsigned int port)
{
    if (MFM_MAX_CONTROLLERS > 1 && port == io_ports[1])
        return 1;
    return 0;
}

static void
mfmhd_write_control(unsigned int port, unsigned char value)
{
    unsigned int ctlr;

    /* Internal callers may enable DMA; ioctl callers are checked separately. */
    value &= MFM_CTL_VALID_MASK;
    ctlr = mfmhd_controller_from_port(port);
    if (ctlr < MFM_MAX_CONTROLLERS)
        mfmhd_control_shadow[ctlr] = value;
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
    if ((!rlen && !wlen) || (rlen && wlen))
        return 0;

    switch (op) {
    case MFM_CMD_READ_SECTORS:
    case MFM_CMD_WRITE_SECTORS:
    case MFM_CMD_READ_SECTOR_BUFFER:
    case MFM_CMD_WRITE_SECTOR_BUFFER:
    case MFM_CMD_READ_LONG:
    case MFM_CMD_WRITE_LONG:
        return 1;
    }
    return 0;
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

#ifdef CONFIG_MFMHD_DIAG_IOCTLS
static unsigned char
mfmhd_read_control_shadow(unsigned int port)
{
    unsigned int ctlr;

    ctlr = mfmhd_controller_from_port(port);
    if (ctlr >= MFM_MAX_CONTROLLERS)
        return MFM_CTL_PIO_POLLED;
    return mfmhd_control_shadow[ctlr];
}
#endif

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

    cmd_control = (unsigned char)(CONFIG_MFMHD_STEP_CODE & MFM_CCB_STEP_MASK);
#if CONFIG_MFMHD_NO_RETRY
    cmd_control |= MFM_CCB_R1_NO_RETRY;
#endif
#if CONFIG_MFMHD_ECC_IMMEDIATE
    cmd_control |= MFM_CCB_R2_IMMEDIATE_ECC;
#endif
    if (mfmhd_slow_profile)
        cmd_control &= ~MFM_CCB_STEP_MASK;
    return mfmhd_sanitize_cmd_control(cmd_control);
}

static void
mfmhd_init_drive_state(int drive)
{
    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return;
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

#ifdef CONFIG_MFMHD_DIAG_IOCTLS
/*
 * Sector-sized ioctl payloads live in mfmhd_bounce, never on the kernel
 * stack.  This small structure exactly matches the bytes before data[] in
 * struct mfmhd_ioc_long.
 */
struct mfmhd_ioc_long_header {
    unsigned short cylinder;
    unsigned char head;
    unsigned char sector;
    unsigned char count;
    unsigned char cmd_control;
    unsigned char csb;
    unsigned char sense[MFM_SENSE_BYTES];
} __attribute__((packed));

struct mfmhd_ioc_status {
    unsigned char csb;
    unsigned char sense[MFM_SENSE_BYTES];
};

#define MFM_LONG_DATA_OFFSET \
    ((unsigned int)offsetof(struct mfmhd_ioc_long, data))
#define MFM_BUFFER_STATUS_OFFSET \
    ((unsigned int)offsetof(struct mfmhd_ioc_buffer, csb))

static int
mfmhd_get_user(void *dst, unsigned int arg, unsigned int len)
{
    if (!arg)
        return -EINVAL;
    return (verified_memcpy_fromfs(dst, (void *)arg, len) == 0) ? 0 : -EFAULT;
}

static int
mfmhd_put_user(unsigned int arg, void *src, unsigned int len)
{
    if (!arg)
        return -EINVAL;
    return (verified_memcpy_tofs((void *)arg, src, len) == 0) ? 0 : -EFAULT;
}

/*
 * Form a 16-bit user-segment offset without wrapping at 64 KiB.  The final
 * comparison uses an exclusive end offset because ELKS verify_area() also
 * checks the byte immediately after the requested range.
 */
static int
mfmhd_user_offset(unsigned int arg, unsigned int offset, unsigned int len,
    unsigned int *result)
{
    unsigned int address;

    if (!arg)
        return -EINVAL;
    if (offset > (unsigned int)(0xffffU - arg))
        return -EFAULT;
    address = arg + offset;
    if (len > (unsigned int)(0xffffU - address))
        return -EFAULT;
    *result = address;
    return 0;
}

static int
mfmhd_get_user_at(void *dst, unsigned int arg, unsigned int offset,
    unsigned int len)
{
    unsigned int address;
    int err;

    err = mfmhd_user_offset(arg, offset, len, &address);
    if (err)
        return err;
    return mfmhd_get_user(dst, address, len);
}

static int
mfmhd_put_user_at(unsigned int arg, unsigned int offset, void *src,
    unsigned int len)
{
    unsigned int address;
    int err;

    err = mfmhd_user_offset(arg, offset, len, &address);
    if (err)
        return err;
    return mfmhd_put_user(address, src, len);
}

static void
mfmhd_fill_ioctl_status(int drive, struct mfmhd_ioc_status *status)
{
    status->csb = drive_info[drive].last_csb;
    memcpy(status->sense, drive_info[drive].last_sense, MFM_SENSE_BYTES);
}
#endif

static const char *
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

static int
mfmhd_controller_is_seagate(void)
{
#if MFMHD_CONTROLLER == MFMHD_CTRL_SEAGATE
    return 1;
#else
    return 0;
#endif
}

static const char *
mfmhd_controller_name(void)
{
    if (mfmhd_controller_is_seagate())
        return "Seagate ST11M/R/FileCard";
    return "WD1002A-WX1";
}

static const char *
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

static void
mfmhd_print_cylinder_value(const char *name, unsigned short value)
{
    if (mfmhd_is_none(value))
        printk(" %s=none", name);
    else
        printk(" %s=%u", name, value);
}

static void
mfmhd_init_ports(void)
{
    io_ports[0] = MFM_HD_PRIMARY_PORT;
    io_ports[1] = MFM_HD_SECONDARY_PORT;
    mfmhd_control_shadow[0] = MFM_CTL_PIO_POLLED;
    mfmhd_control_shadow[1] = MFM_CTL_PIO_POLLED;

    if (MFM_HD_PORT_SWAP) {
        unsigned int swap;

        swap = io_ports[0];
        io_ports[0] = io_ports[1];
        io_ports[1] = swap;
        printk("mfmhd: port mapping swapped (primary=0x%x secondary=0x%x)\n",
            io_ports[0], io_ports[1]);
    }

    /*
     * WD1002 BIOS ROMs may leave the adapter in interrupt/DMA mode.  Claim
     * only controllers selected for probing; an unused 0x324 may belong to
     * unrelated XT hardware.
     */
    mfmhd_write_control(io_ports[0], MFM_CTL_PIO_POLLED);
    if (MFMHD_ACTIVE_DRIVES > MFM_UNITS_PER_CTLR)
        mfmhd_write_control(io_ports[1], MFM_CTL_PIO_POLLED);

    if (MFMHD_ACTIVE_DRIVES > MFM_UNITS_PER_CTLR) {
        if (mfmhd_controller_is_seagate())
            printk("mfmhd: %s profile, ports 0x%x/0x%x\n",
                mfmhd_controller_name(), io_ports[0], io_ports[1]);
        else
            printk("mfmhd: %s profile, BIOS table=%s, ports 0x%x/0x%x\n",
                mfmhd_controller_name(), mfmhd_bios_name(), io_ports[0],
                io_ports[1]);
    } else if (mfmhd_controller_is_seagate()) {
        printk("mfmhd: %s profile, port 0x%x\n",
            mfmhd_controller_name(), io_ports[0]);
    } else {
        printk("mfmhd: %s profile, BIOS table=%s, port 0x%x\n",
            mfmhd_controller_name(), mfmhd_bios_name(), io_ports[0]);
    }
    if (mfmhd_slow_profile)
        printk("mfmhd: slow timing profile enabled\n");
}

static unsigned int
mfmhd_select_port(int drive)
{
    unsigned int ctlr;

    ctlr = (unsigned int)drive >> 1;
    if (ctlr >= MFM_MAX_CONTROLLERS)
        ctlr = 0;
    return io_ports[ctlr];
}

static int
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

static unsigned int
mfmhd_read_s1(unsigned int port)
{
    return (unsigned int)inb_p(port + MFM_HD_JUMPER) & 0xff;
}

/*
 * Calculate heads * sectors with bounded 16-bit additions.  Controller
 * geometry limits make the normal result at most 1024.  Zero reports either
 * invalid input or 16-bit overflow; callers reject both cases.
 */
static unsigned int
mfmhd_sectors_per_cylinder(unsigned int heads, unsigned int sectors)
{
    unsigned int result;

    result = 0;
    while (heads--) {
        if (result > 0xffffU - sectors)
            return 0;
        result += sectors;
    }
    return result;
}

/*
 * Form the two-word sector capacity using 8086-cheap shift-and-add steps.
 * No C wide multiply is emitted.  Each word addition propagates its carry
 * explicitly.  Although valid WD/ST11 geometry cannot overflow the two-word
 * result, malformed future inputs saturate to 0xffff:0xffff rather than wrap.
 */
static sector_t
mfmhd_chs_capacity(unsigned int cylinders, unsigned int heads,
    unsigned int sectors)
{
    union mfm_sector_words result;
    union mfm_sector_words addend;
    unsigned int multiplier;
    unsigned int old_word;
    unsigned int carry;
    unsigned int spc;

    result.word.low = 0;
    result.word.high = 0;
    spc = mfmhd_sectors_per_cylinder(heads, sectors);
    if (!spc || !cylinders)
        return result.value;

    addend.word.low = spc;
    addend.word.high = 0;
    multiplier = cylinders;

    while (multiplier) {
        if (multiplier & 1U) {
            old_word = result.word.low;
            result.word.low += addend.word.low;
            carry = result.word.low < old_word;

            old_word = result.word.high;
            result.word.high += addend.word.high;
            if (result.word.high < old_word)
                goto overflow;
            if (carry) {
                result.word.high++;
                if (!result.word.high)
                    goto overflow;
            }
        }

        multiplier >>= 1;
        if (!multiplier)
            break;
        if (addend.word.high & 0x8000U)
            goto overflow;
        carry = (addend.word.low & 0x8000U) != 0;
        addend.word.high <<= 1;
        if (carry)
            addend.word.high |= 1U;
        addend.word.low <<= 1;
    }
    return result.value;

overflow:
    result.word.low = 0xffffU;
    result.word.high = 0xffffU;
    return result.value;
}

static int
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
    if (lba <= 0xffffUL && spc <= 0xff) {
        unsigned int lba16 = (unsigned int)lba;

        *pcyl = lba16 / spc;
        tmp = lba16 - (*pcyl * spc);
    } else {
        /*
         * sector_t is the existing two-word ELKS block ABI.  __divmod uses
         * two 8086 DIV instructions to divide that word pair by the bounded
         * sectors-per-cylinder value and returns the remainder through spc.
         * Reject a quotient that cannot cross into the 16-bit CHS interface.
         */
        tmp = spc;
        quotient = __divmod(lba, &tmp);
        if (quotient > 0xffffUL)
            return -1;
        *pcyl = (unsigned int)quotient;
    }

    *phead = tmp / sec;
    *psector = tmp - (*phead * sec);
    return 0;
}

static void
mfmhd_set_drive_geometry(int drive, const struct mfm_wd1002_geom *entry,
    unsigned int type_index, unsigned int source)
{
    drive_info[drive].cylinders = entry->cylinders;
    drive_info[drive].heads = entry->heads;
    drive_info[drive].sectors = entry->sectors ?
        entry->sectors : MFM_WD1002_SECTORS;
    drive_info[drive].wp_cylinder = entry->wp_cylinder;
    drive_info[drive].rwc_cylinder = entry->rwc_cylinder;
    drive_info[drive].type_index = (unsigned char)type_index;
    drive_info[drive].source = (unsigned char)source;
}

static int
mfmhd_set_geometry_from_user(int drive)
{
    unsigned int cyl;
    unsigned int heads;
    unsigned int sec;
    struct mfm_wd1002_geom entry;

    cyl = heads = sec = 0;
    switch (drive) {
    case 0:
        cyl = MFMHD_DRIVE0_CYLINDERS;
        heads = MFMHD_DRIVE0_HEADS;
        sec = MFMHD_DRIVE0_SECTORS;
        break;
    case 1:
        cyl = MFMHD_DRIVE1_CYLINDERS;
        heads = MFMHD_DRIVE1_HEADS;
        sec = MFMHD_DRIVE1_SECTORS;
        break;
    case 2:
        cyl = MFMHD_DRIVE2_CYLINDERS;
        heads = MFMHD_DRIVE2_HEADS;
        sec = MFMHD_DRIVE2_SECTORS;
        break;
    case 3:
        cyl = MFMHD_DRIVE3_CYLINDERS;
        heads = MFMHD_DRIVE3_HEADS;
        sec = MFMHD_DRIVE3_SECTORS;
        break;
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
    mfmhd_set_drive_geometry(drive, &entry, 0, MFM_GEO_SRC_USER);

    printk("mfmhd: /dev/mfm%c using user geometry %u/%u/%u\n",
        'a' + (unsigned char)drive, cyl, heads, sec);
    return 1;
}

static const struct mfm_wd1002_geom *
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

static unsigned int
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

static void
mfmhd_set_wd1002_geometry_by_jumper(int drive, unsigned int port)
{
    const struct mfm_wd1002_geom *entry;
    unsigned int raw_s1;
    unsigned int type_index;
    unsigned int table_size;

    raw_s1 = mfmhd_read_s1(port);
    type_index = mfmhd_wd1002_type_from_s1(drive, raw_s1);
    entry = mfmhd_table_entry(type_index, &table_size);
    if (type_index >= table_size)
        type_index = 0;

    mfmhd_set_drive_geometry(drive, entry, type_index, MFM_GEO_SRC_WD1002);

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

static int
mfmhd_wait_status(unsigned int port, unsigned char flags, unsigned char mask,
    jiff_t timeout, const char *where)
{
    jiff_t expiry;
    unsigned char st;

    expiry = jiffies + timeout;
    st = 0;
    do {
        st = STATUS(port);
        if (st == 0xff)
            return -1;
        if ((st & mask) == flags)
            return 0;
    } while (!time_after(jiffies, expiry));

    mfmhd_dump_status(port, where, st);
    return -1;
}

static int
mfmhd_command_expects_data_phase(unsigned char op)
{
    return op == MFM_CMD_READ_SECTORS || op == MFM_CMD_WRITE_SECTORS ||
        op == MFM_CMD_VERIFY_SECTORS || op == MFM_CMD_READ_SECTOR_BUFFER ||
        op == MFM_CMD_WRITE_SECTOR_BUFFER || op == MFM_CMD_READ_LONG ||
        op == MFM_CMD_WRITE_LONG || op == MFM_CMD_READ_ECC_BURST ||
        op == MFM_CMD_FORMAT_DRIVE || op == MFM_CMD_FORMAT_TRACK ||
        op == MFM_CMD_FORMAT_BAD_TRACK ||
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
mfmhd_diag_ticks(void)
{
    return mfmhd_slow_profile ? MFM_SLOW_DIAG_TICKS : MFM_DIAG_TICKS;
}

static jiff_t
mfmhd_command_wait_ticks(unsigned char op)
{
    switch (op) {
    case MFM_CMD_RECALIBRATE:
    case MFM_CMD_SEEK:
    case MFM_CMD_READ_SECTORS:
    case MFM_CMD_WRITE_SECTORS:
    case MFM_CMD_VERIFY_SECTORS:
    case MFM_CMD_READ_LONG:
    case MFM_CMD_WRITE_LONG:
    case MFM_CMD_READ_ECC_BURST:
    case MFM_CMD_INIT_DRIVE_PARAMETERS:
    case MFM_CMD_ST11_GET_GEOMETRY:
        return mfmhd_slow_profile ? MFM_SLOW_DISK_TICKS : MFM_DISK_TICKS;
    case MFM_CMD_BUFFER_DIAGNOSTIC:
    case MFM_CMD_DRIVE_DIAGNOSTIC:
    case MFM_CMD_CONTROLLER_DIAGNOSTIC:
        return mfmhd_diag_ticks();
    case MFM_CMD_FORMAT_DRIVE:
    case MFM_CMD_FORMAT_TRACK:
    case MFM_CMD_FORMAT_BAD_TRACK:
        return MFM_FORMAT_TICKS;
    default:
        return mfmhd_phase_ticks();
    }
}

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
mfmhd_drive_from_port_unit(unsigned int port, unsigned char unit)
{
    unsigned int c;

#if MFMHD_CONTROLLER == MFMHD_CTRL_SEAGATE && MFMHD_ACTIVE_DRIVES == 1
    /*
     * FileCard 20 presents both physical controller units as one logical
     * /dev/mfma.  Attribute second-half completions to that logical drive.
     */
    if (port == io_ports[0] && (unit & 1) &&
            drive_info[0].source == MFM_GEO_SRC_FILECARD20)
        return 0;
#endif
    c = 0;
    if (MFM_MAX_CONTROLLERS > 1 && port == io_ports[1])
        c = 1;
    return (int)(c * MFM_UNITS_PER_CTLR + (unit & 0x01));
}

static void
mfmhd_note_completion(int drive, unsigned char csb, unsigned char *sense)
{
    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return;
    drive_info[drive].last_csb = csb;
    if (sense)
        memcpy(drive_info[drive].last_sense, sense, MFM_SENSE_BYTES);
}

static int
mfmhd_phase_timeout(unsigned int port, int drive, unsigned char op,
    const char *where, unsigned char st)
{
    mfmhd_dump_status(port, where, st);
    mfmhd_debug_set(40, drive, port, -1);
    if (drive >= 0 && drive < MFM_MAX_DRIVES)
        drive_info[drive].cmd_error_count++;
    mfmhd_resync_controller(port, drive, op);
    return -1;
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

static int
mfmhd_cmd_len(unsigned int port, unsigned char *cmdblk, unsigned int cmdlen,
    unsigned char *read, unsigned int rlen, unsigned char *write,
    unsigned int wlen, unsigned char *sense)
{
    unsigned char op;
    unsigned int cmdleft;
    unsigned int inleft;
    unsigned int outleft;
    unsigned int cmd_sent;
    unsigned int data_in;
    unsigned int data_out;
    unsigned int dummy_out;
    unsigned int done_count;
    jiff_t phase_deadline;
    unsigned char phase;
    unsigned char st;
    unsigned char csb;
    unsigned char early_status;
    unsigned char cmd_unit;
    unsigned char dma_active;
    unsigned int dma_len;
    int cmd_drive;
    unsigned char senseblk[MFM_SENSE_BYTES];
    unsigned char *sensebuf;
    unsigned char sensecmd[MFM_COMMAND_BYTES];
#if MFMHD_TRACE
    unsigned int trace_seq;
    unsigned char cmdsave[MFM_TRACE_COMMAND_BYTES];
    unsigned int i;
#endif

    if (!cmdblk || !cmdlen)
        return -1;

    op = cmdblk[0];
    cmd_unit = (unsigned char)((cmdblk[1] >> 5) & 0x07);
    cmd_drive = mfmhd_drive_from_port_unit(port, cmd_unit);
    cmdleft = cmdlen;
    inleft = rlen;
    outleft = wlen;
    dma_active = (unsigned char)mfmhd_command_uses_dma(op, rlen, wlen);
    dma_len = rlen ? rlen : wlen;
    cmd_sent = data_in = data_out = dummy_out = 0;
    csb = 0xff;
    early_status = 0;
    sensebuf = sense ? sense : senseblk;

#if MFMHD_TRACE
    trace_seq = 0;
    if (mfmhd_trace_budget) {
        trace_seq = ++mfmhd_trace_seq;
        mfmhd_trace_budget--;
        for (i = 0; i < MFM_TRACE_COMMAND_BYTES; i++)
            cmdsave[i] = 0;
        for (i = 0; i < cmdlen && i < MFM_TRACE_COMMAND_BYTES; i++)
            cmdsave[i] = cmdblk[i];
        printk("mfmhd-trace[%u]: op=%02x port=0x%x cmdlen=%u cmd=%02x %02x %02x %02x %02x %02x rlen=%u wlen=%u st=%02x\n",
            trace_seq, op, port, cmdlen, cmdsave[0], cmdsave[1],
            cmdsave[2], cmdsave[3], cmdsave[4], cmdsave[5],
            rlen, wlen, STATUS(port));
    }
#endif

    if (dma_active) {
        if (mfmhd_dma_prepare(rlen, write, wlen))
            return -1;
        /* The 8237 owns the payload; the phase loop handles only the CDB/CSB. */
        inleft = 0;
        outleft = 0;
    }

    /* Match Linux xd WD1002 select-then-control ordering. */
    outb_p(0, port + MFM_HD_SELECT);
    mfmhd_write_control(port, dma_active ? MFM_CTL_DRQEN :
        MFM_CTL_PIO_POLLED);

    if (mfmhd_wait_status(port, MFM_STAT_SELECT, MFM_STAT_SELECT,
            mfmhd_select_ticks(), "select/busy timeout")) {
        if (dma_active) {
            mfmhd_dma_stop();
            mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
        }
        return -1;
    }

    phase_deadline = jiffies + mfmhd_phase_ticks();
    while (1) {
        st = STATUS(port);
        if (st == 0xff)
            return mfmhd_phase_timeout(port, cmd_drive, op,
                "floating bus", st);

        if (!(st & MFM_STAT_READY)) {
            if (time_after(jiffies, phase_deadline))
                return mfmhd_phase_timeout(port, cmd_drive, op,
                    "request timeout", st);
            continue;
        }

        phase = st & MFM_STAT_PHASE_MASK;
        switch (phase) {
        case MFM_PHASE_CMD_OUT:
            /* C/D=1 I/O=0: command block bytes, host to controller. */
            if (!cmdleft)
                return mfmhd_phase_timeout(port, cmd_drive, op,
                    "unexpected command-out phase", st);
            DATA_OUT(port, *cmdblk++);
            cmdleft--;
            cmd_sent++;
            phase_deadline = jiffies + (cmdleft ?
                mfmhd_phase_ticks() : mfmhd_command_wait_ticks(op));
            break;

        case MFM_PHASE_DATA_OUT:
            /* C/D=0 I/O=0: write payload, host to controller. */
            if (dma_active)
                break;
            if (cmdleft || !outleft) {
                /*
                 * The Linux xd driver writes a harmless zero whenever an XT
                 * controller asks for data without a host payload.  Some
                 * WD/Xebec cards use this to drain/reset their bus phase.
                 */
                if (++dummy_out > MFM_COMMAND_BYTES)
                    return mfmhd_phase_timeout(port, cmd_drive, op,
                        "unexpected data-out phase", st);
                DATA_OUT(port, 0);
                phase_deadline = jiffies + mfmhd_phase_ticks();
                break;
            }
            done_count = mfmhd_data_out_burst(port, &write, outleft);
            outleft -= done_count;
            data_out += done_count;
            phase_deadline = jiffies + (outleft ?
                mfmhd_phase_ticks() : mfmhd_command_wait_ticks(op));
            break;

        case MFM_PHASE_DATA_IN:
            /* C/D=0 I/O=1: read payload, controller to host. */
            if (dma_active)
                break;
            if (cmdleft || outleft)
                return mfmhd_phase_timeout(port, cmd_drive, op,
                    "data-in before output complete", st);
            if (!inleft)
                return mfmhd_phase_timeout(port, cmd_drive, op,
                    "unexpected data-in phase", st);
            done_count = mfmhd_data_in_burst(port, read ? &read : NULL,
                inleft);
            inleft -= done_count;
            data_in += done_count;
            phase_deadline = jiffies + mfmhd_phase_ticks();
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
    if (dma_active) {
        mfmhd_dma_stop();
        data_in = rlen;
        data_out = wlen;
    }
#if MFMHD_TRACE
    if (trace_seq)
        printk("mfmhd-trace[%u]: csb=%02x st=%02x cmd=%u in=%u out=%u left c/i/o=%u/%u/%u\n",
            trace_seq, csb, STATUS(port), cmd_sent, data_in, data_out,
            cmdleft, inleft, outleft);
#endif

    if (mfmhd_wait_status(port, 0, MFM_STAT_SELECT, mfmhd_select_ticks(),
            "busy clear timeout")) {
        if (cmd_drive >= 0 && cmd_drive < MFM_MAX_DRIVES)
            drive_info[cmd_drive].cmd_error_count++;
        mfmhd_resync_controller(port, cmd_drive, op);
        return -1;
    }
    if (dma_active)
        mfmhd_write_control(port, MFM_CTL_PIO_POLLED);

    if (op == MFM_CMD_SENSE && sense)
        mfmhd_note_completion(cmd_drive, csb, sense);
    else
        mfmhd_note_completion(cmd_drive, csb, NULL);

    if (early_status && !(csb & MFM_CSB_ERROR)) {
        if (cmd_drive >= 0 && cmd_drive < MFM_MAX_DRIVES)
            drive_info[cmd_drive].cmd_error_count++;
        mfmhd_resync_controller(port, cmd_drive, op);
        return -1;
    }

    if (csb & MFM_CSB_ERROR) {
        if (mfmhd_quiet_probe && op == MFM_CMD_TEST_DRIVE_READY) {
            mfmhd_debug_set(42, cmd_drive, port, (int)csb);
            mfmhd_resync_controller(port, cmd_drive, op);
            return -1;
        }
        if (op != MFM_CMD_SENSE) {
            mfmhd_build_command(sensecmd, MFM_CMD_SENSE,
                (unsigned char)((csb & MFM_CSB_LUN) >> 5), 0, 0, 0, 0,
                mfmhd_drive_cmd_control(cmd_drive));
            if (mfmhd_cmd(port, sensecmd, sensebuf, MFM_SENSE_BYTES,
                    NULL, 0, NULL))
                printk("mfmhd: sense command failed after op=0x%02x\n", op);
            else {
                mfmhd_note_completion(cmd_drive, csb, sensebuf);
                printk("mfmhd: sense op=0x%02x bytes=%02x %02x %02x %02x\n",
                    op, sensebuf[0], sensebuf[1], sensebuf[2], sensebuf[3]);
            }
        }
        printk("mfmhd: command 0x%02x failed csb=0x%02x port=0x%x\n",
            op, csb, port);
        mfmhd_debug_set(41, cmd_drive, port, (int)csb);
        if (cmd_drive >= 0 && cmd_drive < MFM_MAX_DRIVES)
            drive_info[cmd_drive].cmd_error_count++;
        mfmhd_resync_controller(port, cmd_drive, op);
        return -1;
    }

    if (dma_active && rlen)
        mfmhd_dma_copy_read(read, dma_len);

    return 0;
}

static int
mfmhd_cmd(unsigned int port, unsigned char *cmdblk, unsigned char *read,
    unsigned int rlen, unsigned char *write, unsigned int wlen,
    unsigned char *sense)
{
    return mfmhd_cmd_len(port, cmdblk, MFM_COMMAND_BYTES, read, rlen,
        write, wlen, sense);
}

static void
mfmhd_build_command(unsigned char *cmdblk, unsigned char op, unsigned char drive,
    unsigned char head, unsigned short cylinder, unsigned char sector,
    unsigned char count, unsigned char cmd_control)
{
    cmdblk[0] = op;
    /* Each controller has two local units; drive 2/3 use unit 0/1 at port 2. */
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

static void
mfmhd_build_setparam_command(unsigned char *cmdblk, unsigned char *params,
    unsigned char drive, unsigned char heads, unsigned short cylinders,
    unsigned short rwc_cylinder, unsigned short wp_cylinder,
    unsigned char ecc, unsigned char cmd_control)
{
    mfmhd_build_command(cmdblk, MFM_CMD_INIT_DRIVE_PARAMETERS, drive, 0, 0, 0,
        0, cmd_control);

    params[0] = (unsigned char)((cylinders >> 8) & 0x03);
    params[1] = (unsigned char)(cylinders & 0xff);
    params[2] = (unsigned char)(heads & 0x1f);
    params[3] = (unsigned char)((rwc_cylinder >> 8) & 0x03);
    params[4] = (unsigned char)(rwc_cylinder & 0xff);
    params[5] = (unsigned char)((wp_cylinder >> 8) & 0x03);
    params[6] = (unsigned char)(wp_cylinder & 0xff);
    params[7] = ecc;
}

static int
mfmhd_set_drive_parameters(unsigned int port, int drive)
{
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned char params[MFM_SETPARAM_PARAMETER_BYTES];
    unsigned short rwc_cylinder;
    unsigned short wp_cylinder;

    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return -1;
    if (!drive_info[drive].heads || !drive_info[drive].cylinders)
        return -1;

    rwc_cylinder = mfmhd_param_cylinder(drive_info[drive].rwc_cylinder);
    wp_cylinder = mfmhd_param_cylinder(drive_info[drive].wp_cylinder);
    mfmhd_build_setparam_command(cmdblk, params, (unsigned char)drive,
        (unsigned char)drive_info[drive].heads,
        (unsigned short)drive_info[drive].cylinders, rwc_cylinder,
        wp_cylinder, MFM_DEFAULT_ECC_LENGTH, mfmhd_drive_cmd_control(drive));

    return mfmhd_cmd(port, cmdblk, NULL, 0, params,
        MFM_SETPARAM_PARAMETER_BYTES, NULL);
}

static int
mfmhd_set_geometry_from_seagate(int drive, unsigned int port)
{
    unsigned char cmdblk[MFM_COMMAND_BYTES];
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

    mfmhd_build_command(cmdblk, MFM_CMD_ST11_GET_GEOMETRY,
        (unsigned char)drive, 0, 0, 0, 1, 0);
    err = mfmhd_cmd(port, cmdblk, (unsigned char *)buf, MFM_SECTOR_BYTES,
        NULL, 0, NULL);
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
    mfmhd_set_drive_geometry(drive, &entry, 0, MFM_GEO_SRC_SEAGATE);

    printk("mfmhd: /dev/mfm%c Seagate controller geometry %u/%u/%u\n",
        'a' + (unsigned char)drive, cyl, heads, sec);
    return 1;
}

static int
mfmhd_set_geometry_from_filecard20(int drive)
{
    struct mfm_wd1002_geom entry;

    if (drive != 0)
        return 0;

    entry.cylinders = MFM_FILECARD20_CYLINDERS;
    entry.heads = MFM_FILECARD20_HEADS;
    entry.sectors = MFM_FILECARD20_SECTORS;
    entry.wp_cylinder = MFMHD_GEO_NONE;
    entry.rwc_cylinder = MFMHD_GEO_NONE;
    mfmhd_set_drive_geometry(drive, &entry, 0, MFM_GEO_SRC_FILECARD20);

    printk("mfmhd: /dev/mfm%c using Seagate FileCard 20 split-unit geometry %u/%u/%u\n",
        'a' + (unsigned char)drive, MFM_FILECARD20_CYLINDERS,
        MFM_FILECARD20_HEADS, MFM_FILECARD20_SECTORS);
    return 1;
}

static void
mfmhd_recalibrate_unit(unsigned int port, int drive, unsigned int unit)
{
    unsigned char cmdblk[MFM_COMMAND_BYTES];

    mfmhd_build_command(cmdblk, MFM_CMD_RECALIBRATE, (unsigned char)unit,
        0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    (void)mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL);
}

static void
mfmhd_recalibrate_drive(unsigned int port, int drive)
{
    mfmhd_recalibrate_unit(port, drive, (unsigned int)drive & 1U);
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
     */
    i = mfmhd_wait_status(port, MFM_STAT_READY, MFM_STAT_READY,
        mfmhd_diag_ticks(), "reset ready timeout");
    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
    mfmhd_debug_set(i ? 22 : 21, -1, port, i ? -1 : 0);
    return i;
}

#ifdef CONFIG_MFMHD_DIAG_IOCTLS
static int
mfmhd_request_sense(unsigned int port, int drive, unsigned char *sense,
    unsigned char *csb)
{
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned char local_sense[MFM_SENSE_BYTES];
    unsigned char *sensebuf;

    sensebuf = sense ? sense : local_sense;
    mfmhd_build_command(cmdblk, MFM_CMD_SENSE, (unsigned char)drive,
        0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    if (mfmhd_cmd(port, cmdblk, sensebuf, MFM_SENSE_BYTES, NULL, 0, sensebuf))
        return -1;

    if (csb)
        *csb = drive_info[drive].last_csb;
    return 0;
}

static int
mfmhd_run_nodata_command(int drive, unsigned char op, unsigned char cmd_control)
{
    unsigned int port;
    unsigned char cmdblk[MFM_COMMAND_BYTES];

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, op, (unsigned char)drive, 0, 0, 0, 0,
        cmd_control);
    return mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL);
}

static int
mfmhd_chs_valid(int drive, unsigned int cylinder, unsigned int head,
    unsigned int sector, unsigned int count)
{
    if (drive < 0 || drive >= MFM_MAX_DRIVES)
        return 0;
    if (cylinder >= drive_info[drive].cylinders)
        return 0;
    if (head >= drive_info[drive].heads)
        return 0;
    if (sector >= drive_info[drive].sectors)
        return 0;
    if (!count || count > MFMHD_MAX_BURST)
        return 0;
    if (sector + count > drive_info[drive].sectors)
        return 0;
    return 1;
}

static int
mfmhd_ioctl_getregs(int drive, unsigned int arg)
{
    struct mfmhd_ioc_regs regs;
    unsigned int port;

    port = mfmhd_select_port(drive);
    memset(&regs, 0, sizeof(regs));
    regs.port = (unsigned short)port;
    regs.status = STATUS(port);
    regs.jumper = (unsigned char)inb_p(port + MFM_HD_JUMPER);
    regs.control = mfmhd_read_control_shadow(port);
    regs.last_csb = drive_info[drive].last_csb;
    memcpy(regs.last_sense, drive_info[drive].last_sense, MFM_SENSE_BYTES);
    regs.retries = drive_info[drive].retry_count;
    regs.cmd_errors = drive_info[drive].cmd_error_count;
    regs.io_errors = drive_info[drive].io_error_count;
    return mfmhd_put_user(arg, &regs, sizeof(regs));
}

static int
mfmhd_ioctl_getsense(int drive, unsigned int arg)
{
    struct mfmhd_ioc_sense sense;
    unsigned int port;
    int err;

    memset(&sense, 0, sizeof(sense));
    port = mfmhd_select_port(drive);
    err = mfmhd_request_sense(port, drive, sense.bytes, &sense.csb);
    if (err)
        sense.csb = drive_info[drive].last_csb;
    if (mfmhd_put_user(arg, &sense, sizeof(sense)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_getopts(int drive, unsigned int arg)
{
    struct mfmhd_ioc_opts opts;
    unsigned int port;

    port = mfmhd_select_port(drive);
    memset(&opts, 0, sizeof(opts));
    opts.cmd_control = mfmhd_drive_cmd_control(drive);
    opts.port_control = mfmhd_read_control_shadow(port);
    opts.force_single = drive_info[drive].force_single;
    return mfmhd_put_user(arg, &opts, sizeof(opts));
}

static int
mfmhd_ioctl_setopts(int drive, unsigned int arg)
{
    struct mfmhd_ioc_opts opts;
    unsigned int port;
    int err;

    err = mfmhd_get_user(&opts, arg, sizeof(opts));
    if (err)
        return err;

    port = mfmhd_select_port(drive);
    drive_info[drive].cmd_control = mfmhd_sanitize_cmd_control(opts.cmd_control);
    drive_info[drive].force_single = opts.force_single ? 1 : 0;

#if CONFIG_MFMHD_UNSAFE_CTRL_IOCTLS
    if (opts.port_control & ~MFM_CTL_VALID_MASK)
        return -EINVAL;
    mfmhd_write_control(port, opts.port_control);
#else
    if (opts.port_control & MFM_CTL_VALID_MASK)
        return -EINVAL;
    mfmhd_write_control(port, MFM_CTL_PIO_POLLED);
#endif
    return 0;
}

static int
mfmhd_ioctl_recalibrate(int drive)
{
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, MFM_CMD_RECALIBRATE, (unsigned char)drive,
        0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    return mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL) ? -EIO : 0;
}

static int
mfmhd_ioctl_seek(int drive, unsigned int arg)
{
    struct mfmhd_ioc_chs chs;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;
    int err;

    err = mfmhd_get_user(&chs, arg, sizeof(chs));
    if (err)
        return err;
    if (chs.cylinder >= drive_info[drive].cylinders ||
            chs.head >= drive_info[drive].heads)
        return -EINVAL;

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, MFM_CMD_SEEK, (unsigned char)drive,
        chs.head, chs.cylinder, 0, 0, chs.cmd_control);
    err = mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL);
    chs.csb = drive_info[drive].last_csb;
    memcpy(chs.sense, drive_info[drive].last_sense, MFM_SENSE_BYTES);
    if (mfmhd_put_user(arg, &chs, sizeof(chs)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_verify(int drive, unsigned int arg)
{
    struct mfmhd_ioc_chs chs;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;
    int err;

    err = mfmhd_get_user(&chs, arg, sizeof(chs));
    if (err)
        return err;
    if (!mfmhd_chs_valid(drive, chs.cylinder, chs.head, chs.sector, chs.count))
        return -EINVAL;

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, MFM_CMD_VERIFY_SECTORS,
        (unsigned char)drive, chs.head, chs.cylinder, chs.sector,
        chs.count, chs.cmd_control);
    err = mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL);
    chs.csb = drive_info[drive].last_csb;
    memcpy(chs.sense, drive_info[drive].last_sense, MFM_SENSE_BYTES);
    if (mfmhd_put_user(arg, &chs, sizeof(chs)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_format(int drive, unsigned int cmd, unsigned int arg)
{
    struct mfmhd_ioc_long_header header;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;
    int err;
    unsigned char op;

    if (cmd == MFMHDIOC_FORMAT_DRIVE)
        op = MFM_CMD_FORMAT_DRIVE;
    else if (cmd == MFMHDIOC_FORMAT_TRACK)
        op = MFM_CMD_FORMAT_TRACK;
    else
        op = MFM_CMD_FORMAT_BAD_TRACK;

    err = mfmhd_get_user(&header, arg, sizeof(header));
    if (err)
        return err;
    /*
     * Format byte 4 is an interleave value, not a transfer count.  Zero is
     * valid; values at or above the sectors-per-track value are not.  These
     * commands address a track and have no host-data phase on WD/Xebec
     * controllers, so the legacy ioctl data area is deliberately ignored.
     */
    if (header.cylinder >= drive_info[drive].cylinders ||
            header.head >= drive_info[drive].heads ||
            header.sector >= drive_info[drive].sectors ||
            header.count >= drive_info[drive].sectors)
        return -EINVAL;

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, op,
        (unsigned char)drive, header.head, header.cylinder, header.sector,
        header.count, header.cmd_control);
    err = mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL);
    header.csb = drive_info[drive].last_csb;
    memcpy(header.sense, drive_info[drive].last_sense, MFM_SENSE_BYTES);
    if (mfmhd_put_user(arg, &header, sizeof(header)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_init_params(int drive, unsigned int arg)
{
    struct mfmhd_ioc_status status;
    unsigned int port;
    int err;

    port = mfmhd_select_port(drive);
    err = mfmhd_set_drive_parameters(port, drive);
    if (arg) {
        if (!mfmhd_bounce)
            return -ENXIO;
        memset(mfmhd_bounce, 0, MFM_SECTOR_BYTES);
        if (mfmhd_put_user_at(arg, 0, mfmhd_bounce, MFM_SECTOR_BYTES))
            return -EFAULT;
        mfmhd_fill_ioctl_status(drive, &status);
        if (mfmhd_put_user_at(arg, MFM_BUFFER_STATUS_OFFSET, &status,
                sizeof(status)))
            return -EFAULT;
    }
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_ecc(int drive, unsigned int arg)
{
    struct mfmhd_ioc_status status;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;
    int err;

    if (!mfmhd_bounce)
        return -ENXIO;
    mfmhd_bounce[0] = 0;

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, MFM_CMD_READ_ECC_BURST,
        (unsigned char)drive, 0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    /* Opcode 0x0d returns one corrected-ECC burst-length byte. */
    err = mfmhd_cmd(port, cmdblk, (unsigned char *)mfmhd_bounce, 1,
        NULL, 0, NULL);
    if (mfmhd_put_user_at(arg, 0, mfmhd_bounce, 1))
        return -EFAULT;
    mfmhd_fill_ioctl_status(drive, &status);
    if (mfmhd_put_user_at(arg, MFM_BUFFER_STATUS_OFFSET, &status,
            sizeof(status)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_long(int drive, unsigned int cmd, unsigned int arg)
{
    struct mfmhd_ioc_long_header header;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;
    int err;
    unsigned char op = (cmd == MFMHDIOC_WRITE_LONG) ?
        MFM_CMD_WRITE_LONG : MFM_CMD_READ_LONG;

    if (!mfmhd_bounce)
        return -ENXIO;
    err = mfmhd_get_user(&header, arg, sizeof(header));
    if (err)
        return err;
    /* READ/WRITE LONG has one 512-byte sector plus exactly four ECC bytes. */
    if (header.count != 1 ||
            !mfmhd_chs_valid(drive, header.cylinder, header.head,
                header.sector, header.count))
        return -EINVAL;
    if (cmd == MFMHDIOC_WRITE_LONG) {
        err = mfmhd_get_user_at(mfmhd_bounce, arg, MFM_LONG_DATA_OFFSET,
            MFM_BOUNCE_BYTES);
        if (err)
            return err;
    } else {
        /* Never disclose stale kernel data if the controller short-reads. */
        memset(mfmhd_bounce, 0, MFM_BOUNCE_BYTES);
    }

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk, op,
        (unsigned char)drive, header.head, header.cylinder, header.sector,
        header.count, header.cmd_control);

    if (cmd == MFMHDIOC_WRITE_LONG)
        err = mfmhd_cmd(port, cmdblk, NULL, 0,
            (unsigned char *)mfmhd_bounce, MFM_BOUNCE_BYTES, NULL);
    else
        err = mfmhd_cmd(port, cmdblk, (unsigned char *)mfmhd_bounce,
            MFM_BOUNCE_BYTES, NULL, 0, NULL);

    if (cmd == MFMHDIOC_READ_LONG &&
            mfmhd_put_user_at(arg, MFM_LONG_DATA_OFFSET, mfmhd_bounce,
                MFM_BOUNCE_BYTES))
        return -EFAULT;
    header.csb = drive_info[drive].last_csb;
    memcpy(header.sense, drive_info[drive].last_sense, MFM_SENSE_BYTES);
    if (mfmhd_put_user(arg, &header, sizeof(header)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_diag(int drive, unsigned char op, unsigned int arg)
{
    struct mfmhd_ioc_sense sense;
    int err;

    memset(&sense, 0, sizeof(sense));
    err = mfmhd_run_nodata_command(drive, op, mfmhd_drive_cmd_control(drive));
    sense.csb = drive_info[drive].last_csb;
    memcpy(sense.bytes, drive_info[drive].last_sense, MFM_SENSE_BYTES);
    if (arg && mfmhd_put_user(arg, &sense, sizeof(sense)))
        return -EFAULT;
    return err ? -EIO : 0;
}

static int
mfmhd_ioctl_buffer(int drive, unsigned int cmd, unsigned int arg)
{
    struct mfmhd_ioc_status status;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned int port;
    int err;

    if (!mfmhd_bounce)
        return -ENXIO;
    if (cmd == MFMHDIOC_WRITE_BUFFER) {
        err = mfmhd_get_user_at(mfmhd_bounce, arg, 0, MFM_SECTOR_BYTES);
        if (err)
            return err;
    } else {
        if (!arg)
            return -EINVAL;
        memset(mfmhd_bounce, 0, MFM_SECTOR_BYTES);
    }

    port = mfmhd_select_port(drive);
    mfmhd_build_command(cmdblk,
        cmd == MFMHDIOC_WRITE_BUFFER ? MFM_CMD_WRITE_SECTOR_BUFFER :
            MFM_CMD_READ_SECTOR_BUFFER,
        (unsigned char)drive, 0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
    err = mfmhd_cmd(port, cmdblk,
        cmd == MFMHDIOC_READ_BUFFER ? (unsigned char *)mfmhd_bounce : NULL,
        cmd == MFMHDIOC_READ_BUFFER ? MFM_SECTOR_BYTES : 0,
        cmd == MFMHDIOC_WRITE_BUFFER ? (unsigned char *)mfmhd_bounce : NULL,
        cmd == MFMHDIOC_WRITE_BUFFER ? MFM_SECTOR_BYTES : 0,
        NULL);
    if (cmd == MFMHDIOC_READ_BUFFER &&
            mfmhd_put_user_at(arg, 0, mfmhd_bounce, MFM_SECTOR_BYTES))
        return -EFAULT;
    mfmhd_fill_ioctl_status(drive, &status);
    if (mfmhd_put_user_at(arg, MFM_BUFFER_STATUS_OFFSET, &status,
            sizeof(status)))
        return -EFAULT;
    return err ? -EIO : 0;
}
#endif

static int
mfmhd_probe_controller(unsigned int port, int drive)
{
    unsigned int attempt;
    unsigned int max_attempts;
    unsigned char st;
    unsigned char cmdblk[MFM_COMMAND_BYTES];

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

        mfmhd_build_command(cmdblk, MFM_CMD_TEST_DRIVE_READY,
            (unsigned char)drive, 0, 0, 0, 0, mfmhd_drive_cmd_control(drive));
        mfmhd_quiet_probe = 1;
        if (!mfmhd_cmd(port, cmdblk, NULL, 0, NULL, 0, NULL)) {
            mfmhd_quiet_probe = 0;
            mfmhd_debug_set(32, drive, port, 0);
            return 0;
        }
        mfmhd_quiet_probe = 0;
        mfmhd_debug_set(33, drive, port, -1);
    }

    if (!(drive & 1))
        printk("mfmhd: /dev/mfm%c test-ready failed at port=0x%x; probing media anyway\n",
            'a' + (unsigned char)drive, port);
    if (!(drive & 1))
        return 0;
    return -1;
}

static int
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

static int
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

static int
mfmhd_vbr_looks_like_fat_volume(const char *b)
{
    if (mfmhd_vbr_has_fat_fs_type(b))
        return 1;
    return mfmhd_vbr_bpb_plausible_fat(b);
}

static int
mfmhd_has_mbr_signature(const char *b)
{
    return b[510] == (char)0x55 && b[511] == (char)0xaa;
}

static int
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
    mfmhd_set_drive_geometry(drive, &entry, 0, MFM_GEO_SRC_BPB);

    printk("mfmhd: /dev/mfm%c FAT BPB geometry %u/%u/%u total=%lu\n",
        'a' + (unsigned char)drive, cyl, heads, sec, (unsigned long)totalsz);
    return 1;
}

static int
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
    mfmhd_set_drive_geometry(drive, &entry, 0, MFM_GEO_SRC_WD1002);

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
    unsigned int port;
    unsigned int retry;
    unsigned int unit;
    unsigned int heads;
    unsigned int sec;
    unsigned int spc;
    unsigned int cyl;
    unsigned int max_cyl;
    unsigned int head;
    unsigned int sector;
    unsigned int track_left;
    sector_t cmd_lba;
    sector_t unit_left;
    unsigned char cmdblk[MFM_COMMAND_BYTES];
    unsigned char sense[MFM_SENSE_BYTES];

    if (drive < 0 || drive >= MFM_MAX_DRIVES || !sectors)
        return -1;

    heads = drive_info[drive].heads;
    sec = drive_info[drive].sectors;
    if (!heads || !sec)
        return -1;

    unit = (unsigned int)drive & 1U;
    cmd_lba = lba;
    max_cyl = drive_info[drive].cylinders;

    if (drive == 0 && drive_info[drive].source == MFM_GEO_SRC_FILECARD20) {
        if (lba >= MFM_FILECARD20_UNIT_SECTORS) {
            unit = 1;
            cmd_lba = lba - MFM_FILECARD20_UNIT_SECTORS;
        }

        max_cyl = MFM_FILECARD20_UNIT_CYLINDERS;
        if (cmd_lba >= MFM_FILECARD20_UNIT_SECTORS)
            return -1;
        unit_left = MFM_FILECARD20_UNIT_SECTORS - cmd_lba;
        if (!unit_left)
            return -1;
        if ((sector_t)sectors > unit_left)
            sectors = (unsigned int)unit_left;
    }

    spc = mfmhd_sectors_per_cylinder(heads, sec);
    if (!spc)
        return -1;

    if (mfmhd_lba_to_chs(cmd_lba, heads, sec, &cyl, &head, &sector))
        return -1;

    if (cyl >= max_cyl || head >= heads || sector >= sec)
        return -1;

    track_left = sec - sector;
    if (sectors > track_left)
        sectors = track_left;
    if (sectors > MFMHD_MAX_BURST)
        sectors = MFMHD_MAX_BURST;
    if (sectors > MFM_DMA_SECTORS)
        sectors = MFM_DMA_SECTORS;
    if (drive_info[drive].force_single && sectors > 1)
        sectors = 1;

    port = mfmhd_select_port(drive);
    for (retry = 0; retry < MFM_RETRIES; retry++) {
        if (retry) {
            drive_info[drive].retry_count++;
            if (!mfmhd_slow_profile || retry == MFM_RETRIES - 1)
                printk("mfmhd: /dev/mfm%c retry %u %s lba=%lu unit=%u n=%u\n",
                    'a' + (unsigned char)drive, retry,
                    write ? "write" : "read", (unsigned long)lba, unit,
                    sectors);
            mfmhd_recalibrate_unit(port, drive, unit);
        }

        mfmhd_build_command(cmdblk,
            write ? MFM_CMD_WRITE_SECTORS : MFM_CMD_READ_SECTORS,
            (unsigned char)unit, (unsigned char)head, (unsigned short)cyl,
            (unsigned char)sector, (unsigned char)sectors,
            mfmhd_drive_cmd_control(drive));

        if (!mfmhd_cmd(port, cmdblk,
                write ? NULL : (unsigned char *)buffer,
                write ? 0 : sectors * MFM_SECTOR_BYTES,
                write ? (unsigned char *)buffer : NULL,
                write ? sectors * MFM_SECTOR_BYTES : 0,
                sense)) {
            if (drive_info[drive].force_single && sectors == 1) {
                if (drive_info[drive].single_ok_streak < 255)
                    drive_info[drive].single_ok_streak++;
                if (drive_info[drive].single_ok_streak >= MFM_SINGLE_RECOVER_OK) {
                    drive_info[drive].force_single = 0;
                    drive_info[drive].single_ok_streak = 0;
                    drive_info[drive].single_restore_count++;
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
        drive_info[drive].single_enable_count++;
        printk("mfmhd: /dev/mfm%c enabling single-sector transfer mode after %s errors\n",
            'a' + (unsigned char)drive, write ? "write" : "read");
    } else {
        drive_info[drive].single_ok_streak = 0;
    }

    printk("mfmhd: /dev/mfm%c %s failed lba=%lu unit=%u chs=%u/%u/%u n=%u\n",
        'a' + (unsigned char)drive, write ? "write" : "read",
        (unsigned long)lba, unit, cyl, head, sector, sectors);
    return -1;
}

static int
mfmhd_probe_drive(int drive)
{
    unsigned int port;
    unsigned int tries;
    char *secbuf;
    int read_ok;
    int read_ok2;
    int fat_vbr;
    char *secbuf2;

    port = mfmhd_select_port(drive);
    mfmhd_debug_set(50, drive, port, 0);
    if (mfmhd_probe_controller(port, drive)) {
        mfmhd_debug_set(51, drive, port, -1);
        return -1;
    }

    if (!mfmhd_set_geometry_from_user(drive)) {
        if (mfmhd_controller_is_seagate()) {
            if (!mfmhd_set_geometry_from_seagate(drive, port) &&
                    !mfmhd_set_geometry_from_filecard20(drive)) {
                printk("mfmhd: /dev/mfm%c has no Seagate geometry\n",
                    'a' + (unsigned char)drive);
            }
        } else {
            mfmhd_set_wd1002_geometry_by_jumper(drive, port);
        }
    }

    if (!mfmhd_geometry_sane(drive_info[drive].cylinders,
            drive_info[drive].heads, drive_info[drive].sectors, 0)) {
        printk("mfmhd: /dev/mfm%c has unusable geometry\n",
            'a' + (unsigned char)drive);
        mfmhd_debug_set(52, drive, port, -1);
        return -1;
    }

    /* Read sector 0 before any WD sector-0 geometry fallback. */
    mfmhd_recalibrate_drive(port, drive);

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
        mfmhd_recalibrate_drive(port, drive);
    }

    if (!read_ok) {
        printk("mfmhd: /dev/mfm%c unable to read sector 0\n",
            'a' + (unsigned char)drive);
        mfmhd_debug_set(54, drive, port, -1);
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
        drive_info[drive].probe_mismatch_count++;
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
        if (mfmhd_set_drive_parameters(port, drive)) {
            printk("mfmhd: /dev/mfm%c unable to program WD1002 parameters\n",
                'a' + (unsigned char)drive);
            mfmhd_debug_set(56, drive, port, -1);
            heap_free(secbuf2);
            heap_free(secbuf);
            return -1;
        }
        printk("mfmhd: /dev/mfm%c programmed WD1002 parameters %u/%u/%u\n",
            'a' + (unsigned char)drive, drive_info[drive].cylinders,
            drive_info[drive].heads, drive_info[drive].sectors);
        mfmhd_recalibrate_drive(port, drive);
    }

    heap_free(secbuf2);
    heap_free(secbuf);
    mfmhd_debug_set(55, drive, port, 0);
    return 0;
}

struct gendisk * INITPROC mfmhd_init(void)
{
    int drive;
    int i;
    int p;
    int hdcnt;
    int hdmax;
    int ctrl;
    sector_t sectors;

    if (dev_disabled(DEV_MFMA)) {
        printk("mfmhd: disabled\n");
        return NULL;
    }

    mfmhd_init_ports();
    mfmhd_debug_set(10, -1, io_ports[0], 0);
    memset(&mfmhd_part[0], 0xff, sizeof(mfmhd_part));
    for (i = 0; i < (MFM_MAX_DRIVES << MFM_MINOR_SHIFT); i++) {
        mfmhd_part[i].start_sect = NOPART;
        mfmhd_part[i].nr_sects = 0;
    }

    hdcnt = 0;
    hdmax = -1;
    for (drive = 0; drive < MFM_MAX_DRIVES && drive < MFMHD_ACTIVE_DRIVES; drive++) {
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
        printk("mfmhd: /dev/mfm%c health retries=%u cmd_err=%u io_err=%u single_on=%u single_off=%u probe_mismatch=%u\n",
            'a' + (unsigned char)drive,
            drive_info[drive].retry_count, drive_info[drive].cmd_error_count,
            drive_info[drive].io_error_count, drive_info[drive].single_enable_count,
            drive_info[drive].single_restore_count, drive_info[drive].probe_mismatch_count);
    }

    if (!hdcnt) {
        printk("mfmhd: no XT MFM drives found\n");
        mfmhd_debug_set(90, -1, io_ports[0], -1);
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

    for (p = 0; p < mfmhd_gendisk.nr_hd; p++) {
        if (mfmhd_part[p << MFM_MINOR_SHIFT].start_sect == NOPART ||
            mfmhd_part[p << MFM_MINOR_SHIFT].nr_sects == 0)
            continue;
        ctrl = p >> 1;
        if (ctrl >= MFM_MAX_CONTROLLERS)
            ctrl = 0;
        printk("mfmhd: /dev/mfm%c port=0x%x irq=%d masked dma=3 polled\n",
            'a' + (unsigned char)p, io_ports[ctrl], mfmhd_irq_map[ctrl]);
    }

    printk("mfmhd: found %d hard drive%c\n", hdcnt, hdcnt == 1 ? ' ' : 's');
    mfmhd_debug_set(91, -1, io_ports[0], 0);
    return &mfmhd_gendisk;
}

static int
mfmhd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
    unsigned int arg)
{
#ifdef CONFIG_MFMHD_DIAG_IOCTLS
    unsigned int port;
#endif
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
#ifdef CONFIG_MFMHD_DIAG_IOCTLS
    case MFMHDIOC_GETREGS:
        return mfmhd_ioctl_getregs(drive, arg);
    case MFMHDIOC_GETSENSE:
        return mfmhd_ioctl_getsense(drive, arg);
    case MFMHDIOC_TEST_READY:
        return mfmhd_ioctl_diag(drive, MFM_CMD_TEST_DRIVE_READY, arg);
    case MFMHDIOC_RECALIBRATE:
        return mfmhd_ioctl_recalibrate(drive);
    case MFMHDIOC_SEEK:
        return mfmhd_ioctl_seek(drive, arg);
    case MFMHDIOC_VERIFY:
        return mfmhd_ioctl_verify(drive, arg);
    case MFMHDIOC_CTRL_DIAG:
        return mfmhd_ioctl_diag(drive, MFM_CMD_CONTROLLER_DIAGNOSTIC, arg);
    case MFMHDIOC_BUFFER_DIAG:
        return mfmhd_ioctl_diag(drive, MFM_CMD_BUFFER_DIAGNOSTIC, arg);
    case MFMHDIOC_DRIVE_DIAG:
        return mfmhd_ioctl_diag(drive, MFM_CMD_DRIVE_DIAGNOSTIC, arg);
    case MFMHDIOC_FORMAT_DRIVE:
    case MFMHDIOC_FORMAT_TRACK:
    case MFMHDIOC_FORMAT_BAD_TRACK:
        return mfmhd_ioctl_format(drive, cmd, arg);
    case MFMHDIOC_INIT_PARAMS:
        return mfmhd_ioctl_init_params(drive, arg);
    case MFMHDIOC_READ_ECC_BURST:
        return mfmhd_ioctl_ecc(drive, arg);
    case MFMHDIOC_READ_LONG:
    case MFMHDIOC_WRITE_LONG:
        return mfmhd_ioctl_long(drive, cmd, arg);
    case MFMHDIOC_RESET:
        port = mfmhd_select_port(drive);
        return mfmhd_reset_controller(port) ? -EIO : 0;
    case MFMHDIOC_GETOPTS:
        return mfmhd_ioctl_getopts(drive, arg);
    case MFMHDIOC_SETOPTS:
        return mfmhd_ioctl_setopts(drive, arg);
    case MFMHDIOC_READ_BUFFER:
    case MFMHDIOC_WRITE_BUFFER:
        return mfmhd_ioctl_buffer(drive, cmd, arg);
#endif
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
            if (xfer < 0) {
                drive_info[drive].io_error_count++;
                printk("mfmhd: io error drive=%d start=%lu cmd=%s\n", drive,
                    (unsigned long)start, req->rq_cmd == WRITE ? "write" : "read");
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
            end_request(1);
        }
    }
}
