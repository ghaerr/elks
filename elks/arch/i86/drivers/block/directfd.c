/*
 * Copyright (C) 1991, 1992  Linus Torvalds
 *
 * 02.12.91 - Changed to static variables to indicate need for reset
 * and recalibrate. This makes some things easier (output_byte reset
 * checking etc), and means less interrupt jumping in case of errors,
 * so the code is hopefully easier to understand.
 *
 * This file is certainly a mess. I've tried my best to get it working,
 * but I don't like programming floppies, and I have only one anyway.
 * Urgel. I should check for more errors, and do more graceful error
 * recovery. Seems there are problems with several drives. I've tried to
 * correct them. No promises.
 *
 * As with hd.c, all routines within this file can (and will) be called
 * by interrupts, so extreme caution is needed. A hardware interrupt
 * handler may not sleep, or a kernel panic will happen. Thus I cannot
 * call "floppy-on" directly, but have to set a special timer interrupt
 * etc.
 *
 * 28.02.92 - made track-buffering routines, based on the routines written
 * by entropy@wintermute.wpi.edu (Lawrence Foard). Linus.
 *
 * Automatic floppy-detection and formatting written by Werner Almesberger
 * (almesber@nessie.cs.id.ethz.ch), who also corrected some problems with
 * the floppy-change signal detection.
 *
 * 1992/7/22 -- Hennus Bergman: Added better error reporting, fixed
 * FDC data overrun bug, added some preliminary stuff for vertical
 * recording support.
 *
 * 1992/9/17: Added DMA allocation & DMA functions. -- hhb.
 *
 * TODO: Errors are still not counted properly.
 *
 * 1992/9/20
 * Modifications for ``Sector Shifting'' by Rob Hooft (hooft@chem.ruu.nl)
 * modelled after the freeware MS/DOS program fdformat/88 V1.8 by
 * Christoph H. Hochstatter.
 *
 * I have fixed the shift values to the ones I always use. Maybe a new
 * ioctl() should be created to be able to modify them.
 * There is a bug in the driver that makes it impossible to format a
 * floppy as the first thing after bootup.
 *
 * 1993/4/29 -- Linus -- cleaned up the timer handling in the kernel, and
 * this helped the floppy driver as well. Much cleaner, and still seems to
 * work.
 *
 * 1996/3/27 -- trp@cyberoptics.com
 * began port to linux-8086
 * removed volatile and fixed function definitions
 *
 * 2023/03/10 -- helge@skrivervik.com
 * rewritten for TLVC - imported DMA setup from Minix for simplicity and
 * compactness. Added fix for bioses that do motor timeout on their own.
 * Now compiles - and works.
 *
 * Sep 2023 - Greg Haerr
 * Ported Helge's modifications to ELKS. Lots of original driver cleanup.
 * Expanded FDC and platform identification to allow running on original
 * IBM PC and clones with no DIR register. Added lots of info and error msgs.
 *
 * Mar 2024 - Greg Haerr
 * Added dynamic floppy bounce buffer
 *
 * 22 May 2025 - Greg Haerr
 * Program 8237 DMA controller external page register instead of using XMS bounce buffer
 */

#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/stat.h>
#include <linuxmt/kernel.h>
#include <linuxmt/timer.h>
#include <linuxmt/mm.h>
#include <linuxmt/signal.h>
#include <linuxmt/fd.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/heap.h>
#include <linuxmt/genhd.h>
#include <linuxmt/devnum.h>
#include <linuxmt/debug.h>

#include <arch/dma.h>
#include <arch/fdreg.h>
#include <arch/system.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/segment.h>
#include <arch/ports.h>

#define MAJOR_NR        FLOPPY_MAJOR
#include "blk.h"

#ifndef CONFIG_ASYNCIO
#error  Direct FD driver requires CONFIG_ASYNCIO
#endif

//#define DEBUG printk
#define DEBUG(...)

/*
 * The original 8272A doesn't have FD_DOR, FD_DIR or FD_CCR registers,
 * but the 82077 found on modern clones can be configured in hardware
 * to emulate the chip and adaptor found on many PC/AT compatibles.
 *
 * History and capabilities of FDC chips on IBM Compatibles (all have MSR,DATA regs)
 *  Chip         System             Adaptor Regs    Chip Regs    New Commands
 *  8272A/NEC765 IBM PC, IBM PC/XT  DOR
 *  8272A/NEC765 IBM PC/AT          DOR,DIR,CCR,DSR
 *  8272A/NEC765 IBM PS/2 (gen 1)   DOR,DIR,CCR,DSR
 *  82072        IBM PS/2 (gen 2)                   DSR          CONFIGURE,DUMPREGS
 *  82077AA      IBM PS/2 (gen 3)                   DOR,DIR,CCR  PERPENDICULAR,LOCK
 */

char USE_IMPLIED_SEEK = 0; /* =1 for QEMU with 360k/AT stretch floppies (not real hw) */
#define CHECK_DIR_REG       1   /* =1 to read and clear DIR DSKCHG when media changed */
#define CHECK_DISK_CHANGE   1   /* =1 to inform kernel of media changed */
#define CACHE_CYLINDER      1   /* =1 to cache to end of cylinder rather than track */
#define CACHE_FULL_TRACK    0   /* =1 to read full tracks when track caching */
#define TRACK_SPLIT_BLK     1   /* =1 to read extra sector on track split block */
#define MOTORDELAY          0   /* =1 to emulate motor on delay for floppy on QEMU */
#define IODELAY             0   /* =1 to emulate delay for floppy on QEMU */

#define CACHE_CYLINDER_MAX  12  /* max sectors to cache when CACHE_CYLINDER */
#define CACHE_SIZE          (TRACKSEGSZ >> 9)   /* max usable track cache in sectors */

/* adjustable timeouts */
#define TIMEOUT_MOTOR_ON   (HZ/2)      /* 500 ms wait for floppy motor on before I/O */
#define TIMEOUT_MOTOR_OFF  (3 * HZ)    /* 3 secs wait for floppy motor off after I/O */
#define TIMEOUT_CMD_COMPL  (6 * HZ)    /* 6 secs wait for FDC command complete */

/* default locations for cache and bounce buffers */
#define BOUNCE_SEG      TRACKSEG /* share bounce buffer with track cache at TRACKSEG:0 */
#define BOUNCE_OFF      0
#define CACHE_SEG       TRACKSEG /* track cache at TRACKSEG:0 (same as BIOS FD driver) */
#define CACHE_OFF       0

#define FLOPPY_DMA      2       /* hardwired on old PCs */

#define abs(v)          (((int)(v) >= 0)? (v): -(v))

#define bool unsigned char      /* don't require stdbool.h yet */

static void (*do_floppy)();     /* interrupt routine to call */
static bool initial_reset_flag;
static bool need_configure = 1; /* for 82077 */
static bool reset;              /* something went wrong, reset FDC, start over */
static bool recalibrate;        /* seek errors, etc, eventually triggers recalibrate_floppy */
static bool recover;            /* recovering from hang, awakened by watchdog timer */
static bool seek;               /* set if current op needs a track change (seek) */

/* BIOS floppy motor timeout counter - FIXME leave this while BIOS driver present */
static unsigned char __far *fl_timeout = (void __far *)0x440L;

/* NOTE: current_DOR tells which motor(s) have been commanded to run,
 * 'running' tells which ones are actually running. The difference is subtle,
 * with typical spinup time of .5 secs.
 */
static unsigned char current_DOR;
static unsigned char running;   /* keep track of motors already running */
/*
 * Note that MAX_ERRORS=X doesn't imply that we retry every bad read
 * max X times - some types of errors increase the errorcount by 2 or
 * even 3, so we might actually retry only X/2 times before giving up.
 */
#define MAX_ERRORS      6

/*
 * Threshold for reporting FDC errors to the console.
 * Setting this to zero may flood your screen when using
 * ultra cheap floppies ;-)
 */
#define MIN_ERRORS      0

#define LINADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 4) + (unsigned)(offs)))
#define XMSADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 0) + (unsigned)(offs)))

/*
 * globals used by 'result()'
 */
#define MAX_REPLIES 7
static unsigned char reply_buffer[MAX_REPLIES];
#define ST0 (reply_buffer[0])
#define ST1 (reply_buffer[1])
#define ST2 (reply_buffer[2])
#define ST3 (reply_buffer[3])

/* CMOS drive types, from CMOS location 0x10 */
#define CMOS_NONE   0
#define CMOS_360k   1
#define CMOS_1200k  2
#define CMOS_720k   3
#define CMOS_1440k  4
#define CMOS_2880k  5
#define CMOS_MAX    5

/*
 * Minor number based formats. Each drive type is specified starting
 * from minor number 2 plus the table index, and no auto-probing is used.
 *
 * Stretch tells if the tracks need to be doubled for some
 * types (ie 360kB diskette in 1.2MB drive etc).
 *
 * Rate is 0 for 500kb/s, 1 for 300kbps, 2 for 250kbps, 3 for 1M (+0x40 perpendicular).
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc).
 * H is head unload time, time the FDC will wait before unloading
 * the head after a command that accesses the disk (1=16ms, 2=32ms, etc).
 *
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).
 */

/* indices into minor_types[], used for floppy format probes */
#define FT_360k_PC  1           /* 360kB PC diskettes */
#define FT_1200k    2           /* 1.2 MB AT-diskettes */
#define FT_360k_AT3 3           /* 360kB in 720kB drive */
#define FT_720k     4           /* 3.5" 720kB diskette */
#define FT_360k_AT5 5           /* 360kB in 1.2MB drive */
#define FT_720k_AT5 6           /* 720kB in 1.2MB drive */
#define FT_1440k    7           /* 3.5" 1.44MB diskette */
#define FT_2880k    8           /* 3.5" 2.88MB diskette */

static struct floppy_struct minor_types[] = {
      {  0,   0, 0,  0, 0, 0x00, 0x00, 0x00, 0x00, NULL},
/*1*/ { 720,  9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "360k/PC"},  /* 360kB PC diskettes */
/*2*/ {2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, "1.2M"},     /* 1.2 MB AT-diskettes */
/*3*/ { 720,  9, 2, 40, 1, 0x2A, 0x02, 0xDF, 0x50, "360k/AT3"}, /* 360kB in 720kB drive */
/*4*/ {1440,  9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k"},     /* 3.5" 720kB diskette */
/*5*/ { 720,  9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, "360k/AT"},  /* 360kB in 1.2MB drive */
/*6*/ {1440,  9, 2, 80, 0, 0x23, 0x01, 0xDF, 0x50, "720k/1.2M"},/* 720kB in 1.2MB drive */
/*7*/ {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, "1.44M"},    /* 3.5" 1.44MB diskette */
/*8*/ {5760, 36, 2, 80, 0, 0x1B, 0x43, 0xAF, 0x28, "2.88M"},    /* 3.5" 2.88MB diskette */
    /* totSectors/sectors/heads/tracks/stretch/gap/rate/spec1/fmtgap/name */
};

#define ARRAYLEN(a)     (sizeof(a)/sizeof(a[0]))
#define MAX_MINOR       (ARRAYLEN(minor_types) * 2) /* max forced type minor number */

/* floppy probes to try per CMOS floppy type */
static unsigned char p360k[] =  { FT_360k_PC, FT_360k_AT5, 0 };
//static unsigned char p1200k[] = { FT_1200k,   FT_1440k, FT_360k_PC, FT_360k_AT5, 0 };
static unsigned char p1200k[] = { FT_1200k,   FT_360k_AT5, 0 };
static unsigned char p720k[] =  { FT_720k,    FT_360k_AT3, 0 };
static unsigned char p1440k[] = { FT_1440k,   FT_720k,     0 };
static unsigned char p2880k[] = { FT_2880k,   FT_1440k,    0 };

/*
 * Auto-detection. Each drive type has a zero-terminated list of formats which
 * are used in succession to try to read the disk. If the FDC cannot lock onto
 * the disk, the next format is tried. This uses the variable 'probing'.
 */
static unsigned char *probe_list[CMOS_MAX] = { p360k, p1200k, p720k, p1440k, p2880k };

/*
 * The following variables must exist in duality, one for each drive.
 */

/* Auto-detection: disk type determined from CMOS and probing */
static struct floppy_struct *current_type[2];
static unsigned char *base_type[2];     /* initial probe per drive */
static int fd_ref[2];                   /* ref count for invalidating buffers */
static int fd_probe[2];                 /* set by open routine to start probing */
static struct inode *fd_inode[2];       /* for inode->i_size set after probe */

/*
 * These are global variables, as that's the easiest way to pass info
 * to interrupts. They are the data used for the current request.
 */
#define NO_TRACK 255

static int access_count;            /* ref count for deregistering driver */
static int probing;                 /* set when determining media format */
static bool use_cache;              /* expand read request to fill cache when set */
static int use_bounce;              /* XMS I/O or 64k address wrap when set */
static unsigned char cache_drive = 255;
static unsigned int cache_start;    /* logical sector number */
static unsigned int cache_numsectors; /* size of cache in sectors */
ramdesc_t df_cache_seg = CACHE_SEG; /* track cache segment or linear address if XMS */
static int cur_spec1 = -1;
static int cur_rate = -1;
static struct floppy_struct *floppy;
static unsigned char current_drive = 255;
static unsigned char sector;	    /* zero relative requested I/O sector */
static unsigned int numsectors;     /* # sectors in I/O */
static unsigned char startsector;   /* zero relative actual I/O start sector */
static unsigned char head;
static unsigned char track;
static unsigned char seek_track;
static unsigned char current_track = NO_TRACK;
static unsigned char command;
static unsigned char fdc_version;

static void DFPROC floppy_ready(void);
static void DFPROC redo_fd_request(void);
static void recal_interrupt(void);
static void floppy_shutdown(void);
static void motor_off_callback(int);
static int DFPROC floppy_register(void);
static void DFPROC floppy_deregister(void);
static void floppy_release(struct inode *inode, struct file *filp);

static void DFPROC delay_loop(int cnt)
{
    while (cnt-- > 0) asm("nop");
}

static void invalidate_cache(void)
{
    if (cache_drive != 255) debug_cache("INV%d ", cache_drive);
    cache_drive = 255;
}

static void select_callback(int unused)
{
    DEBUG("SC;");
    floppy_ready();
}

static struct timer_list select = { NULL, 0, 0, select_callback };
/*
 * Select drive for this op - always one at a time, leave motor_on alone,
 * several drives can have the motor running at the same time. A drive cannot
 * be selected unless the motor is on, they can be set concurrently.
 */
static void DFPROC floppy_select(unsigned int nr)
{
    DEBUG("sel0x%x-", current_DOR);
    if (nr == (current_DOR & 3)) {
        /* Drive already selected, we're ready to go */
        floppy_ready();
        return;
    }
    /* activate a different drive */
    seek = 1;
    current_track = NO_TRACK;
    /* 
     * NOTE: This is drive select, not motor on/off
     * Unless the motor is turned on, select will fail 
     * Setting them concurrently is OK
     */
    current_DOR &= 0xFC;
    current_DOR |= nr;
    outb(current_DOR, FD_DOR);

    /* Some FDCs require a delay when changing the current drive */
    del_timer(&select);
    select.tl_expires = jiffies + 2;
    add_timer(&select);
}

static void motor_on_callback(int nr)
{
    DEBUG("mON,");
    clr_irq();  // Experimental
    running |= 0x10 << nr;
    set_irq();
    floppy_select(nr);
}

static struct timer_list motor_on_timer[2] = {
    {NULL, 0, 0, motor_on_callback},
    {NULL, 0, 1, motor_on_callback}
};
static struct timer_list motor_off_timer[2] = {
    {NULL, 0, 0, motor_off_callback},
    {NULL, 0, 1, motor_off_callback}
};
static struct timer_list fd_timeout = {NULL, 0, 0, floppy_shutdown};

static void motor_off_callback(int nr)
{
    unsigned char mask = ~(0x10 << nr);

    DEBUG("[%u]MOF%d", (unsigned int)jiffies, nr);
    clr_irq();
    running &= mask;
    current_DOR &= mask;
    outb(current_DOR, FD_DOR);
    set_irq();
}

/*
 * floppy_on: turn on motor. If already running, call floppy_select
 * otherwise start motor and set timer and return.
 * In the latter case, motor_on_callback will call floppy_select();
 *
 * DOR (Data Output Register) is |MOTD|MOTC|MOTB|MOTA|DMA|/RST|DR1|DR0|
 */
static void DFPROC floppy_on(int nr)
{
    unsigned char mask = 0x10 << nr;    /* motor on select */

    DEBUG("flpON");
    *fl_timeout = 0; /* Reset BIOS motor timeout counter, neccessary on some machines */
    del_timer(&motor_off_timer[nr]);

    if (mask & running) {
        floppy_select(nr);
        DEBUG("#%x;",current_DOR);
        return;
    }

    if (!(mask & current_DOR)) {        /* motor not running yet */
        del_timer(&motor_on_timer[nr]);
        /* TEAC 1.44M says 'waiting time' 505ms, may be too little for 5.25in drives. */
        motor_on_timer[nr].tl_expires = jiffies +
            ((running_qemu && !MOTORDELAY)? 0: TIMEOUT_MOTOR_ON);
        add_timer(&motor_on_timer[nr]);

        current_DOR &= 0xFC;            /* remove drive select */
        current_DOR |= mask;            /* set motor select */
        current_DOR |= nr;              /* set drive select */
        outb(current_DOR, FD_DOR);
    }

    DEBUG("flpON0x%x;", current_DOR);
}

/*
 * floppy_off is called from upper layer routines when a block request
 * is ended. It's extremely important that the motor off timer is slow
 * enough not to affect performance. 3 seconds is a fair compromise.
 */
static void floppy_off(int nr)
{
    del_timer(&motor_off_timer[nr]);
    motor_off_timer[nr].tl_expires = jiffies + TIMEOUT_MOTOR_OFF;
    add_timer(&motor_off_timer[nr]);
    DEBUG("flpOFF-\n");
}

void request_done(int uptodate)
{
    del_timer(&fd_timeout);
    end_request(uptodate);
}

/* The IBM PC can perform DMA operations by using the DMA chip.  To use it,
 * the DMA (Direct Memory Access) chip is loaded with the 20-bit memory address
 * to be read from or written to, the byte count minus 1, and a read or write
 * opcode.  This routine sets up the DMA chip.  Note that the chip is not
 * capable of doing a DMA across a 64K boundary (e.g., you can't read a
 * 512-byte block starting at physical address 65520).
 *
 * The 8237 DMA controller cannot access data above 1MB on the original PC
 * (4 bit page register). The AT is different, the page reg is 8 bits while the
 * 2nd DMA controller transfers words only, not bytes and thus up to 128k bytes
 * in one 'swoop'.
 */
#define DMA_INIT        DMA1_MASK_REG
#define DMA_FLIPFLOP    DMA1_CLEAR_FF_REG
#define DMA_MODE        DMA1_MODE_REG
#define DMA_TOP         DMA_PAGE_2
#define DMA_ADDR        ((FLOPPY_DMA << 1) + 0 + IO_DMA1_BASE)
#define DMA_COUNT       ((FLOPPY_DMA << 1) + 1 + IO_DMA1_BASE)

static void DFPROC setup_DMA(void)
{
    struct request *req = CURRENT;
    unsigned int count, physaddr, use_xms;
    unsigned long dma_addr;

    count = numsectors << 9;
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
    use_xms = req->rq_seg >> 16;
    if (use_xms)
        use_bounce = 0;                 /* XMS buffers also always 1K aligned */
    else {
        physaddr = (req->rq_seg << 4) + (unsigned int)req->rq_buffer;
        use_bounce = (physaddr + count) < physaddr;
    }
    if (!use_cache) {                   /* use_cache overrides use_bounce */
        if (use_bounce) {
            dma_addr = LINADDR(BOUNCE_SEG, BOUNCE_OFF);
            if (command == FD_WRITE) {
                xms_fmemcpyw(BOUNCE_OFF, BOUNCE_SEG, req->rq_buffer, req->rq_seg,
                    BLOCK_SIZE/2);
            }
#if (BOUNCE_SEG == CACHE_SEG) && (BOUNCE_OFF == CACHE_OFF)
            invalidate_cache();
#endif
        }
        else if (use_xms)
             dma_addr = XMSADDR(req->rq_seg, req->rq_buffer);
        else dma_addr = LINADDR(req->rq_seg, req->rq_buffer);
    } else {
        if (df_cache_seg >> 16)          /* XMS cache */
            dma_addr = df_cache_seg;
        else dma_addr = LINADDR(df_cache_seg, CACHE_OFF);
    }
    DEBUG("DMA %c cache %d, bounce %d, count %d req %04x:%04x dma %08lx\n",
        (command == FD_WRITE)? 'W': 'R', use_cache, use_bounce, count,
        (unsigned short)req->rq_seg, req->rq_buffer, dma_addr);

    clr_irq();
    outb(FLOPPY_DMA | 4, DMA_INIT);     /* disable floppy dma channel */
    outb(0, DMA_FLIPFLOP);              /* reset flip flop */
    outb(FLOPPY_DMA | (command==FD_READ? DMA_MODE_READ : DMA_MODE_WRITE), DMA_MODE);
    outb((unsigned) dma_addr >> 0,   DMA_ADDR);
    outb((unsigned) dma_addr >> 8,   DMA_ADDR);
    outb((unsigned)(dma_addr >> 16), DMA_TOP);
    count--;
    outb(count >> 0, DMA_COUNT);
    outb(count >> 8, DMA_COUNT);
    outb(FLOPPY_DMA, DMA_INIT);         /* enable channel */
    set_irq();
}

static void DFPROC output_byte(char byte)
{
    int counter;
    unsigned char status;

    if (reset)
        return;
    for (counter = 0; counter < 10000; counter++) {
        status = inb_p(FD_STATUS) & (STATUS_READY | STATUS_DIR);
        if (status == STATUS_READY) {
            outb(byte, FD_DATA);
            return;
        }
    }
    current_track = NO_TRACK;
    reset = 1;
    printk("df: can't send to FDC\n");
}

static int DFPROC result(void)
{
    int i = 0, counter, status;
    if (reset)
        return -1;
    for (counter = 0; counter < 10000; counter++) {
        status = inb_p(FD_STATUS) & (STATUS_DIR | STATUS_READY | STATUS_BUSY);
        if (status == STATUS_READY) {   /* done, no more result bytes */
            return i;
        }
        if (status == (STATUS_DIR | STATUS_READY | STATUS_BUSY)) {
            if (i >= MAX_REPLIES) {
                printk("floppy_stat reply overrun\n");
                break;
            }
            reply_buffer[i++] = inb_p(FD_DATA);
        }
    }
    reset = 1;
    current_track = NO_TRACK;
    printk("df: result timeout\n");
    return -1;
}

static void DFPROC bad_flp_intr(void)
{
    int errors;

    DEBUG("bad_flpI-");
    current_track = NO_TRACK;
    invalidate_cache();
    if (!CURRENT) return;
    if (probing) {
        probing++;
        return;
    }
    errors = ++CURRENT->rq_errors;
    if (errors >= MAX_ERRORS) {
        printk("df: Max retries #%d exceeded\n", errors);
        request_done(0);
        /* don't reset/recalibrate now as extra interrupt
         * confuses redo_fd_request if no more I/O scheduled.
         */
        return;
    }
    if (errors >= MAX_ERRORS / 2)
        reset = 1;
    else
        recalibrate = 1;
}

/* Set perpendicular mode as required, based on data rate, if supported.
 * 1Mbps data rate only possible with 82077-1.
 */
static void DFPROC perpendicular_mode(unsigned char rate)
{
    if (fdc_version >= FDC_TYPE_82077) {
        output_byte(FD_PERPENDICULAR);
        if (rate & 0x40) {
            unsigned char r = rate & 0x03;
            if (r == 0)
                output_byte(2); /* perpendicular, 500 kbps */
            else if (r == 3)
                output_byte(3); /* perpendicular, 1Mbps */
            else {
                printk("df: Invalid data rate for perpendicular mode!\n");
                reset = 1;
            }
        } else
            output_byte(0);     /* conventional mode */
    } else {
        if (rate & 0x40) {
            printk("df: need 82077 FDC for disc\n");
            reset = 1;
        }
    }
}

static void DFPROC configure_fdc_mode(void)
{
    /* check for enhanced chip with FIFO, vertical recording and implied seeks */
    if (fdc_version >= FDC_TYPE_82072 && (need_configure || USE_IMPLIED_SEEK)) {
        /* implied seek required for QEMU to work with 360k floppies in 1.2M drives */
        int implied_seek = USE_IMPLIED_SEEK && floppy->stretch;
        output_byte(FD_CONFIGURE);
        output_byte(0);
        if (implied_seek)
            output_byte(0x4A); /* FIFO on, polling on, 10 byte threshold, implied seek */
        else
            output_byte(0x0A);      /* FIFO on, polling on, 10 byte threshold */
        output_byte(0);             /* precompensation from track 0 upwards */
        if (need_configure)
            DEBUG("df: implied seek %s\n", implied_seek? "enabled": "disabled");
        need_configure = 0;
    }
    if (cur_spec1 != floppy->spec1) {
        cur_spec1 = floppy->spec1;
        output_byte(FD_SPECIFY);
        output_byte(cur_spec1); /* hut etc */
        output_byte(6);         /* Head load time =6ms, DMA */
    }
    if (cur_rate != floppy->rate) {
        /* use bit 6 of floppy->rate to indicate perpendicular mode */
        perpendicular_mode(floppy->rate);
        outb_p((cur_rate = (floppy->rate)) & ~0x40, FD_CCR);
    }
}

static void DFPROC tell_sector(int nr)
{
    if (nr != 7) {
        printk(" -- FDC reply error");
        reset = 1;
    } else
        printk(": track %d, head %d, sector %d", reply_buffer[3],
               reply_buffer[4], reply_buffer[5]);
}

/*
 * Ok, this interrupt is called after a DMA read/write has succeeded
 * or failed, so we check the results, and copy any buffers.
 * hhb: Added better error reporting.
 */
static void rw_interrupt(void)
{
    struct request *req = CURRENT;
    int nr, bad;
    unsigned int start;
    char *cache_offset;

    nr = result();
    /* NOTE: If cache is active and sector count is uneven, ST0 will
     * always show HD1 as selected at this point. */
    DEBUG("rwI%x|%x|%x-", ST0,ST1,ST2);

    /* check FDC to find cause of interrupt */
    switch ((ST0 & ST0_INTR) >> 6) {
    case 1:                     /* error occured during command execution */
        bad = 1;
        printk("df%d: ", ST0 & ST0_DS);
        if (ST1 & ST1_WP) {
            printk(" write protected");
            request_done(0);
            bad = 0;
        } else if (ST1 & ST1_OR) {
            printk("data overrun");
            /* could continue from where we stopped, but ... */
            bad = 0;
        } else if (req->rq_errors >= MIN_ERRORS) {
            if (ST0 & ST0_ECE) {
                printk("recalibrate failed");
            } else if (ST2 & ST2_CRC) {
                printk("data CRC error");
                tell_sector(nr);
            } else if (ST1 & ST1_CRC) {
                printk("CRC error");
                tell_sector(nr);
            } else if ((ST1 & (ST1_MAM | ST1_ND)) || (ST2 & ST2_MAM)) {
                if (!probing) {
                    printk("sector not found");
                    tell_sector(nr);
                } else
                    printk("probe failed on %s", floppy->name);
            } else if (ST2 & ST2_WC) {  /* seek error */
                printk("wrong cylinder");
            } else if (ST2 & ST2_BC) {  /* cylinder marked as bad */
                printk("bad cylinder");
            } else {
                printk("unknown error, %x %x %x %x", ST0, ST1, ST2, ST3);
            }
        }
        printk("\n");
        if (bad)
            bad_flp_intr();
        redo_fd_request();
        return;
    case 2:                     /* invalid command given */
        DEBUG("df: Invalid FDC command\n");
        request_done(0);
        redo_fd_request();
        return;
    case 3:
        printk("df: Abnormal cmd termination\n");
        bad_flp_intr();
        redo_fd_request();
        return;
    default:                    /* (0) Normal command termination */
        break;
    }

    if (probing) {
        nr = DEVICE_NR(req->rq_dev);
        printk("df%d: floppy type %s\n", nr, floppy->name);
        current_type[nr] = floppy;
        fd_inode[nr]->i_size = (sector_t)floppy->size << 9;
        probing = 0;
    }
    if (use_cache) {
        cache_drive = current_drive;    /* cache now valid */
        start = (unsigned int)req->rq_sector;
        cache_start = ((CACHE_FULL_TRACK && floppy->sect < CACHE_SIZE)?
            start - sector: start);
        cache_numsectors = numsectors;
        cache_offset = (char *)(((start - cache_start) << 9) + CACHE_OFF);
        DEBUG("rd %08lx:%04x->%08lx:%04x;", (unsigned long)df_cache_seg, cache_offset,
                (unsigned long)req->rq_seg, req->rq_buffer);
        xms_fmemcpyw(req->rq_buffer, req->rq_seg, cache_offset, df_cache_seg,BLOCK_SIZE/2);
    } else if (use_bounce && command == FD_READ) {
        xms_fmemcpyw(req->rq_buffer, req->rq_seg, BOUNCE_OFF, BOUNCE_SEG, BLOCK_SIZE/2);
    }
    request_done(1);
    DEBUG("RQOK;");
    redo_fd_request();  /* Continue with the next request - if any */
}

/*
 * We try to read tracks, but if we get too many errors, we go back to
 * reading just one sector at a time. This means we should be able to
 * read a sector even if there are other bad sectors on this track.
 *
 * The FDC will start read/write at the specified sector, then continues
 * until the DMA controller tells it to stop ... as long as we're on the same cyl.
 * Notably: If the # of sectors per track is odd, we read sectors + 1 if head = 0
 * to ensure we have full blocks in the buffer.
 *
 * From the Intel 8272A app note: "The 8272A always operates in a multi-sector 
 * transfer mode. It continues to transfer data until the TC input is active."
 * IOW: We tell the FDC where to start, and the DMA controller where to stop.
 */
static void DFPROC setup_rw_floppy(void)
{
    DEBUG("setup_rw-");

#if IODELAY
    if (running_qemu) {
        static unsigned lasttrack;
        unsigned ms = abs(track - lasttrack) * 4 / 10;
        lasttrack = track;
        if (current_drive == 0)
            ms += 10 + numsectors;    /* 1440k @300rpm = 100ms + ~10ms/sector + 4ms/tr */
        else
            ms += 8 + (numsectors<<1); /* 360k @360rpm = 83ms + ~20ms/sector + 3ms/tr */
        unsigned long timeout = jiffies + ms*HZ/100;
        while (!time_after(jiffies, timeout)) continue;
    }
#endif
    debug_cache("%s%d %lu(CHS %u,%u,%u-%u)\n",
        use_cache? "TR": (command == FD_WRITE? "WR": "RD"),
        current_drive, CURRENT->rq_sector>>1, track, head,
        startsector + 1, startsector + numsectors);
    do_floppy = rw_interrupt;
    setup_DMA();
    output_byte(command);
    output_byte(head << 2 | current_drive);
    output_byte(track);
    output_byte(head);
    output_byte(startsector+1);
    output_byte(2);             /* sector size = 512 */
    output_byte(floppy->sect);
    output_byte(floppy->gap);
    output_byte(0xFF);          /* sector size, 0xff unless sector size==0 (128b) */
    if (reset)                  /* if output_byte timed out */
        redo_fd_request();
}

/*
 * This is the routine called after every seek (or recalibrate) interrupt
 * from the floppy controller. Note that the "unexpected interrupt" routine
 * also does a recalibrate, but doesn't come here.
 */
static void seek_interrupt(void)
{
    /* sense drive status */
    DEBUG("seekI-");
    output_byte(FD_SENSEI);
    if (result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track) {
        printk("df%d: seek failed %x %x %x\n", current_drive, ST0, ST1, seek_track);
        recalibrate = 1;
        bad_flp_intr();
        redo_fd_request();
        return;
    }
    current_track = ST1;
    setup_rw_floppy();
}

/*
 * This routine is called when everything should be correctly set up
 * for the transfer (ie floppy motor is on and the correct floppy is
 * selected, error conditions cleared).
 */
static void DFPROC transfer(void)
{
    DEBUG("trns%d-", use_cache);
    configure_fdc_mode();   /* ensure controller is in the right mode per transaction */
    if (reset) {            /* if output_byte timed out */
        redo_fd_request();
        return;
    }
    if (!seek || (USE_IMPLIED_SEEK && floppy->stretch && fdc_version >= FDC_TYPE_82072)) {
        current_track = seek_track;
        setup_rw_floppy();
        return;
    }

    /* OK; need to change tracks ... */
    do_floppy = seek_interrupt;
    DEBUG("sk%d;",seek_track);
    output_byte(FD_SEEK);
    output_byte((head << 2) | current_drive);
    output_byte(seek_track);
    if (reset)                  /* if output_byte timed out */
        redo_fd_request();
}

static void DFPROC recalibrate_floppy(void)
{
    DEBUG("recal-");
    recalibrate = 0;
    current_track = 0;
    do_floppy = recal_interrupt;
    output_byte(FD_RECALIBRATE);
    output_byte(current_drive);

#if 0
    /* FIXME this may not make sense: We're waiting for recal_interrupt
     * why redo_fd_request here when recal_interrupt is doing it ???
     * 'reset' gets set in recal_interrupt, maybe that's it ???  */
    if (reset)
        redo_fd_request();
#endif
}

/*
 * Special case - used after a unexpected interrupt (or reset)
 */

static void recal_interrupt(void)
{
    output_byte(FD_SENSEI);
    current_track = NO_TRACK;
    if (result() != 2 || (ST0 & 0xE0) == 0x60) /* look for SEEK END and ABN TERMINATION*/
        reset = 1;
    DEBUG("recalI-%x", ST0);    /* Should be 0x20, Seek End */
    /* Recalibrate until track 0 is reached. Might help on some errors. */
    if (ST0 & 0x10) {           /* Look for UnitCheck, which will happen regularly
                                 * on 80 track drives because RECAL only steps 77 times */
        recalibrate_floppy();   /* FIXME: may loop, should limit nr of recalibrates */
    } else
        redo_fd_request();
}

static void unexpected_floppy_interrupt(void)
{
    printk("df: unexpected interrupt\n");
    current_track = NO_TRACK;
    output_byte(FD_SENSEI);
    if (result() != 2 || (ST0 & 0xE0) == 0x60)
        reset = 1;
    else
        recalibrate = 1;
}

/*
 * Must do 4 FD_SENSEI's after reset because of ``drive polling''.
 */
static void reset_interrupt(void)
{
    int i;

    DEBUG("rstI-");
    for (i = 0; i < 4; i++) {
        output_byte(FD_SENSEI);
        result();
    }
    output_byte(FD_SPECIFY);
    output_byte(cur_spec1);     /* hut etc */
    output_byte(6);             /* Head load time =6ms, DMA */
    configure_fdc_mode();       /* reprogram fdc */
    if (initial_reset_flag) {
        initial_reset_flag = 0;
        recalibrate = 1;
        reset = 0;
        return;
    }
    if (!recover)
        redo_fd_request();
    else {
        recalibrate_floppy();
        recover = 0;
    }
}

/*
 * reset is done by pulling bit 2 of DOR low for a while.
 * As the FDC recovers from reset, it pulls the IRQ, and reset_interrupt is called.
 */
static void DFPROC reset_floppy(void)
{
    DEBUG("[%u]rst-", (unsigned int)jiffies);
    do_floppy = reset_interrupt;
    reset = 0;
    current_track = NO_TRACK;
    cur_spec1 = -1;
    cur_rate = -1;
    recalibrate = 1;
    need_configure = 1;         /* not required if LOCK set on 82077 */
    if (!initial_reset_flag)
        DEBUG("df: reset_floppy\n");
    clr_irq();                  /* FIXME don't busyloop with interrupts off, use timer */
    outb_p(current_DOR & ~0x04, FD_DOR);
    delay_loop(1000);
    outb(current_DOR, FD_DOR);
    set_irq();
}

/*
 * shutdown is called by the 'main' watchdog timer (typically 6 secs of idle time)
 * and sets the 'recover' flag to enable a full restart of the adapter: Reset FDC,
 * re-configure, recalibrate, essentially start from scratch.
 */
static void floppy_shutdown(void)
{
    DEBUG("[%u]shtdwn0x%x|%x-", (unsigned int)jiffies, current_DOR, running);
    do_floppy = NULL;
    printk("df%d: FDC cmd timeout\n", current_drive);
    recover = 1;
    reset_floppy();
    redo_fd_request();
}

#if CHECK_DIR_REG
static void shake_done(void)
{
    /* Need SENSEI to clear the interrupt */
    output_byte(FD_SENSEI);
    result();
    DEBUG("shD0x%x-", ST0);
    current_track = NO_TRACK;
    if (inb(FD_DIR) & 0x80) {
        printk("df%d: disk changed, aborting\n", current_drive);
        request_done(0);
    }
    if (ST0 & 0xC0) {
        printk("df%d: error %02x after SENSEI\n", current_drive, ST0 & 0xC0);
        //request_done(0);
    }
    redo_fd_request();
}

static int DFPROC retry_recal(void (*proc)())
{
    DEBUG("rrecal-");
    output_byte(FD_SENSEI);
    if (result() == 2 && (ST0 & 0x10) != 0x10) /* track 0 test */
        return 0;               /* No 'unit check': We're OK */
    do_floppy = proc;           /* otherwise repeat recalibrate */
    output_byte(FD_RECALIBRATE);
    output_byte(current_drive);
    return 1;
}

static void shake_zero(void)
{
    DEBUG("sh0-");
    if (!retry_recal(shake_zero))
        shake_done();
}

static void shake_one(void)
{
    DEBUG("sh1-");
    if (retry_recal(shake_one))
        return;                 /* just return if retry_recal initiated a RECAL */
    do_floppy = shake_done;
    output_byte(FD_SEEK);
    output_byte(head << 2 | current_drive);
    output_byte(1);             /* resetting media change bit requires head movement */
}
#endif /* CHECK_DIR_REG */

#if CHECK_DISK_CHANGE
/*
 * This routine checks whether a removable media has been changed,
 * invalidates all inode and buffer-cache-entries, unmounts
 * any mounted filesystem and closes the driver.
 * It is called from device open, mount and file read/write code.
 * Because the FDC is interrupt driven and can't sleep, this routine
 * must be called by a non-interrupt routine, as invalidate_buffers
 * may sleep. Even without that, all accessed kernel variables would need
 * protection via interrupt disabling.
 *
 * Since this driver is the only driver implementing media change,
 * the entire routine has been moved here for simplicity.
 */
#if DEBUG_EVENT
static int debug_changed;
void fake_disk_change(void)
{
    debug("X");
    debug_changed = 1;
}
#endif

/* bit vector set when media changed - causes I/O to be discarded until unset */
static unsigned int changed_floppies;

int check_disk_change(kdev_t dev)
{
    unsigned int mask;
    struct super_block *s;
    struct inode *inodep;

    if (!dev && changed_floppies) {
        dev = (changed_floppies & 1)? MKDEV(MAJOR_NR, 0): MKDEV(MAJOR_NR, 1);
    } else if (MAJOR(dev) != MAJOR_NR)
        return 0;
    debug("C%d", dev&3);
    mask = 1 << DEVICE_NR(dev);
    if (!(changed_floppies & mask)) {
        debug("N");
        return 0;
    }
    debug("Y");

    if (dev == ROOT_DEV) panic("Root media changed");

    /* I/O is ignored but inuse, locked, or dirty inodes may not be cleared... */
    s = get_super(dev);
    if (s && s->s_mounted) {
        do_umount(dev);
        printk("VFS: Unmounting %s, media changed\n", s->s_mntonname);
        /* fake up inode to enable device close */
        inodep = new_inode(NULL, S_IFBLK);
        inodep->i_rdev = dev;
        floppy_release(inodep, NULL);
        iput(inodep);
    }
    fsync_dev(dev);
    invalidate_inodes(dev);
    invalidate_buffers(dev);

    changed_floppies &= ~mask;
    debug_changed = 0;
    printk("VFS: Disk media change completed on dev %D\n", dev);

    return 1;
}
#endif /* CHECK_DISK_CHANGE */

static void DFPROC floppy_ready(void)
{
    DEBUG("RDY%d,%d-", reset, recalibrate);

#if CHECK_DISK_CHANGE
    /* check if disk changed since last cmd (PC/AT+) */
    if (0
#if DEBUG_EVENT
        || (debug_changed && current_drive == 1)
#endif
#if CHECK_DIR_REG
        || (fdc_version >= FDC_TYPE_8272PC_AT && (inb(FD_DIR) & 0x80))
#endif
                                                ) {
        /* first time through the FDC requires recalibrate in order to clear DIR,
         * so just recalibrate instead of starting to discard I/O in that case.
         */
        if (current_type[current_drive]) {
            changed_floppies |= 1 << current_drive; /* this will discard any queued I/O */
            printk("df%d: Disk media change detected, suspending I/O\n", current_drive);
            current_type[current_drive] = NULL;     /* comment out to keep last media format */
        }

        if (current_drive == cache_drive)
            invalidate_cache();

#if CHECK_DIR_REG
        if (!reset && !recalibrate) {
            if (current_track && current_track != NO_TRACK)
                do_floppy = shake_zero;
            else
                do_floppy = shake_one;
            output_byte(FD_RECALIBRATE);
            output_byte(current_drive);
            return;
        }
#endif
        redo_fd_request();
        return;
    }
#endif /* CHECK_DISK_CHANGE */

    if (reset) {
        reset_floppy();
        return;
    }
    if (recalibrate) {
        recalibrate_floppy();
        return;
    }
    transfer();
}

static void DFPROC redo_fd_request(void)
{
    struct request *req;
    int type, drive;
    unsigned int start, tmp;
    char *cache_offset;

  repeat:
    req = CURRENT;
    if (!req) {
        do_floppy = NULL;
        return;
    }
    CHECK_REQUEST(req);

    seek = 0;
    type = MINOR(req->rq_dev);
    drive = DEVICE_NR(type);
    if (fd_probe[drive]) {
        probing = 1;
        fd_probe[drive] = 0;
    }

#if CHECK_DISK_CHANGE
    if (changed_floppies & (1 << drive)) {
        req->rq_errors = -1;    /* stop error display */
        request_done(0);
        goto repeat;
    }
#endif

    if (type > 1 && type < MAX_MINOR)
        floppy = &minor_types[type >> 1];
    else {                      /* Auto-detection */
        floppy = current_type[drive];
        if (!floppy) {
            tmp = probing? base_type[drive][probing-1]: base_type[drive][0];
            if (!tmp) {
                printk("df%d: Unable to determine drive type\n", drive);
                request_done(0);
                fd_probe[drive] = 1;
                goto repeat;
            }
            floppy = &minor_types[tmp];
            if (!recalibrate && probing > 1)
                printk("df%d: auto-probe #%d %s\n", drive, probing, floppy->name);
        }
    }
    DEBUG("[%u]redo-%c %d(%s) bl %u;", (unsigned int)jiffies, 
        req->rq_cmd == WRITE? 'W':'R', drive, floppy->name, req->rq_sector);

    if (current_drive != drive) {
        current_track = NO_TRACK;
        current_drive = drive;
    }
    start = (unsigned int) req->rq_sector;
    if (start + req->rq_nr_sectors > floppy->size) {
        printk("df%d: sector %u beyond max %u\n", drive, start, floppy->size);
        request_done(0);
        goto repeat;
    }
    sector = start % floppy->sect;
    tmp = start / floppy->sect;
    head = tmp % floppy->head;
    track = tmp / floppy->head;
    seek_track = track << floppy->stretch;
    command = (req->rq_cmd == READ)? FD_READ: FD_WRITE;
    startsector = sector;
    numsectors = req->rq_nr_sectors;
#ifdef CONFIG_TRACK_CACHE
    use_cache = (command == FD_READ) && (req->rq_errors < 4)
        ; //&& (arch_cpu != CPU_80386 || running_qemu); /* disable cache on 32-bit systems */
    if (use_cache) {
        /* full track caching only if cache large enough */
        if (CACHE_FULL_TRACK && floppy->sect < CACHE_SIZE)
            startsector = 0;
#if CACHE_CYLINDER
        /* cache to end of cylinder, max CACHE_CYLINDER_MAX sectors */
        numsectors = (floppy->sect << 1) - (sector + head*floppy->sect);
        if (numsectors > CACHE_CYLINDER_MAX)
            numsectors = CACHE_CYLINDER_MAX;
#elif TRACK_SPLIT_BLK
        /* If the floppy sector count is odd, we cache one more sector
         * when head=0 to get an even number of sectors (full blocks).
         */
        numsectors = floppy->sect + (floppy->sect & 1 && !head) - startsector;
#else
        /* partial track caching */
        numsectors = floppy->sect - startsector;
#endif
        if (numsectors > CACHE_SIZE)
            numsectors = CACHE_SIZE;
    }
#endif

    DEBUG("df%d: %s sector %d CHS %d/%d/%d max %d stretch %d seek %d\n",
        DEVICE_NR(req->rq_dev), req->rq_cmd==READ? "read": "write",
        start, track, head, sector+1, floppy->sect, floppy->stretch, seek_track);

    DEBUG("prep %u|%d,%d|%d-", start, seek_track, cache_drive, current_drive);

    if (cache_drive == current_drive &&
        start >= cache_start && start < cache_start + cache_numsectors) {
        DEBUG("cache CHS %d/%d/%d\n", seek_track, head, sector);
        debug_cache2("CH %d ", start >> 1);
        cache_offset = (char *)(((start - cache_start) << 9) + CACHE_OFF);
        if (command == FD_READ) {       /* cache hit, no I/O necessary */
            xms_fmemcpyw(req->rq_buffer, req->rq_seg, cache_offset, df_cache_seg,
                BLOCK_SIZE/2);
            request_done(1);
            goto repeat;
        } else if (command == FD_WRITE) /* update track buffer, then write */
            xms_fmemcpyw(cache_offset, df_cache_seg, req->rq_buffer, req->rq_seg,
                BLOCK_SIZE/2);
    } 

    /* restart timer for hung operations, 6 secs probably too long ... */
    del_timer(&fd_timeout);
    fd_timeout.tl_expires = jiffies + TIMEOUT_CMD_COMPL;
    add_timer(&fd_timeout);

    if (seek_track != current_track)
        seek = 1;

    floppy_on(current_drive);
}

void do_fd_request(void)
{
    redo_fd_request();
}

static int fd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
    unsigned int arg)
{
    int type, drive, err;
    struct floppy_struct *fp;
    struct hd_geometry *loc;

    if (!inode || !inode->i_rdev)
        return -EINVAL;
    type = MINOR(inode->i_rdev);
    drive = DEVICE_NR(type);
    if (type > 1 && type < MAX_MINOR)
        fp = &minor_types[type >> 1];
    else
        fp = current_type[drive];
    if (!fp)
        return -ENODEV;

    switch (cmd) {
    case HDIO_GETGEO:   /* need this one for the sys/makeboot command */
        loc = (struct hd_geometry *)arg;
        err = verify_area(VERIFY_WRITE, (void *)loc, sizeof(struct hd_geometry));
        if (!err) {
            put_user_char(fp->head, &loc->heads);
            put_user_char(fp->sect, &loc->sectors);
            put_user(fp->track, &loc->cylinders);
            put_user_long(0L, &loc->start);
        }
        return err;
    default:
        return -EINVAL;
    }
}

static int INITPROC CMOS_READ(int addr)
{
    outb_p(addr, 0x70);
    return inb_p(0x71);
}

static unsigned char * INITPROC find_base(int drive, int type)
{
    unsigned char *base;

    if (type > 0 && type <= CMOS_MAX) {
        base = probe_list[type - 1];
        printk("df%d is %s (%d)", drive, minor_types[*base].name, type);
        return base;
    }
    printk("df%d is unknown (%d)", drive, type);
    return NULL;
}

static void INITPROC config_types(void)
{
    printk("df: CMOS ");
    if (!(base_type[0] = find_base(0, (CMOS_READ(0x10) >> 4) & 0xF)))
        base_type[0] = find_base(0, CMOS_360k);   /* use 360k if no CMOS */
    if (((CMOS_READ(0x14) >> 6) & 1) != 0) {
        printk(", ");
        base_type[1] = find_base(1, CMOS_READ(0x10) & 0xF);
    }
#ifdef CONFIG_FS_XMS_BUFFER
    if (xms_enabled) {
        /* df_cache_seg allocated in buffer_init to prevent any 64k DMA wrap */
        printk(", xms cache %dK at %08lx", CACHE_SIZE >> 1, df_cache_seg);
    }
#endif
    printk("\n");
}

static int floppy_open(struct inode *inode, struct file *filp)
{
    int type, drive, err;

    type = MINOR(inode->i_rdev);
    drive = DEVICE_NR(type);

#if CHECK_DISK_CHANGE
    debug_setcallback(2, fake_disk_change);     /* debug trigger ^P */
    if (filp && filp->f_mode) {
        if (check_disk_change(inode->i_rdev))
            return -ENXIO;
    }
#endif

    fd_probe[drive] = 0;
    if (type > 1 && type < MAX_MINOR)           /* forced floppy type */
        floppy = &minor_types[type >> 1];
    else {                      /* Auto-detection */
        floppy = current_type[drive];
        if (!floppy) {
            if (!base_type[drive])
                return -ENXIO;
            if (sys_caps & CAP_PC_AT)           /* don't probe XT systems */
                fd_probe[drive] = 1;
            floppy = &minor_types[base_type[drive][0]];
        }
    }

    if (access_count == 0) {
        err = floppy_register();
        if (err) return err;
        invalidate_cache();
    }

    access_count++;
    fd_ref[drive]++;
    fd_inode[drive] = inode;
    inode->i_size = (sector_t)floppy->size << 9; /* NOTE: may change value after probe */
    DEBUG("df%d: open %s, size %lu\n", drive, floppy->name, inode->i_size);

    return 0;
}

static void floppy_release(struct inode *inode, struct file *filp)
{
    kdev_t dev = inode->i_rdev;
    int drive = DEVICE_NR(dev);

    DEBUG("df%d release\n", drive);
    if (--fd_ref[drive] == 0) {
        fsync_dev(dev);
        invalidate_inodes(dev);
        invalidate_buffers(dev);
    }
    if (--access_count == 0)
        floppy_deregister();
}

static struct file_operations floppy_fops = {
    NULL,                       /* lseek - default */
    block_read,                 /* read - general block-dev read */
    block_write,                /* write - general block-dev write */
    NULL,                       /* readdir - bad */
    NULL,                       /* select */
    fd_ioctl,                   /* ioctl */
    floppy_open,                /* open */
    floppy_release,             /* release */
};

/*
 * The version command is not supposed to generate an interrupt, but
 * my FDC does, except when booting in SVGA screen mode.
 * When it does generate an interrupt, it doesn't return any status bytes.
 * It appears to have something to do with the version command...
 *
 * This should never be called, because of the reset after the version check.
 */
static void ignore_interrupt(void)
{
    printk("df: interrupt ignored (%d)\n", result());
    reset = 1;
    do_floppy = NULL;           /* ignore only once */
}

static void floppy_interrupt(int irq, struct pt_regs *regs)
{
    void (*handler) () = do_floppy;

    do_floppy = NULL;
    if (!handler)
        handler = unexpected_floppy_interrupt;
    handler();
}

/* non-portable, should use int_vector_set and new function get_int_vector */
#define FLOPPY_VEC      (_MK_FP(0, (FLOPPY_IRQ+8) << 2))
static __u32 old_floppy_vec;

static void DFPROC floppy_deregister(void)
{
    outb(0x0c, FD_DOR);         /* all motors off, enable IRQ and DMA */
    clr_irq();
    free_irq(FLOPPY_IRQ);
    *(__u32 __far *)FLOPPY_VEC = old_floppy_vec;
    enable_irq(FLOPPY_IRQ);
    set_irq();
}

/* Try to determine the floppy controller type */
static int DFPROC get_fdc_version(void)
{
    int type = FDC_TYPE_8272A;
    const char *name;

    do_floppy = ignore_interrupt;
    output_byte(FD_VERSION);    /* get FDC version code */
    if (result() != 1) {
        printk("df: can't get FDC version\n");
        return 0;
    }
    switch (reply_buffer[0]) {
    case 0x80:
        if (arch_cpu >= CPU_80286) {     /* PC/AT or better */
            type = FDC_TYPE_8272PC_AT;
            name = "8272A (PC/AT)";
        } else {
            name = "8272A";
        }
        break;
    case 0x90:
        type = FDC_TYPE_82077;
        name = "82077";
        break;
    default:
        name = "Unknown";
    }
    printk("df: direct floppy FDC %s (0x%x), irq %d, dma %d\n",
        name, reply_buffer[0], FLOPPY_IRQ, FLOPPY_DMA);

    /* Not all FDCs seem to be able to handle the version command
     * properly, so force a reset for the standard FDC clones,
     * to avoid interrupt garbage.
     */
    initial_reset_flag = 1;
    reset_floppy();

    return type;
}

static int DFPROC floppy_register(void)
{
    int err;

    current_DOR = 0x0c;
    outb(0x0c, FD_DOR);         /* all motors off, enable IRQ and DMA */

    old_floppy_vec = *((__u32 __far *)FLOPPY_VEC);
    err = request_irq(FLOPPY_IRQ, floppy_interrupt, INT_GENERIC);
    if (err) {
        printk("df: IRQ %d busy\n", FLOPPY_IRQ);
        return err;
    }

    fdc_version = get_fdc_version();
    if (!fdc_version) return -EIO;
    return 0;
}

void INITPROC floppy_init(void)
{
    if (dev_disabled(DEV_DF0)) {
        printk("df0: disabled\n");
        return;
    }
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &floppy_fops))
        return;
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

    if (!USE_IMPLIED_SEEK)
        USE_IMPLIED_SEEK = running_qemu;
    config_types();
}
