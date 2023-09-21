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
 */

/*
 * TODO (HS 2023):
 * - Change read buffer logic to allow small floppies (720k and less) to fill 
 *   the track buffer (full cylinder)
 * - When XMS buffers are active, the BIOS hd driver will use DMASEG as a bounce buffer
 *   thus colliding with the usage here. This is a problem only in the odd case 
 *   that we're using BIOS HD + DIRECT FD + XMS buffers + TRACK cache, 
 *   which really should not happen. IOW - use either BIOS block IO or DIRECT block IO,
 *   don't mix!!
 * - Test density detection logic & floppy change detection
 * - Clean up debug output
 */

#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/timer.h>
#include <linuxmt/mm.h>
#include <linuxmt/signal.h>
#include <linuxmt/fdreg.h>
#include <linuxmt/fd.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>

#include <arch/dma.h>
#include <arch/system.h>
#include <arch/io.h>
#include <arch/segment.h>
#include <arch/ports.h>
#include <arch/hdreg.h>		/* for ioctl GETGEO */

#define FLOPPYDISK
#define MAJOR_NR FLOPPY_MAJOR
#define MINOR_SHIFT	0	/* shift to get drive num */
#define FLOPPY_DMA 2		/* hardwired on old PCs */

#include "blk.h"		/* ugly - blk.h contains code */

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

#define CHECK_DISK_CHANGE   0   /* =1 to add driver media changed code */
#define CLEAR_DIR_REG       0   /* =1 to clear DIR DSKCHG when set (for media change) */

//#define DEBUG printk
#define DEBUG(...)

static void (*do_floppy)();     /* interrupt routine to call */
static int initial_reset_flag;
static int need_configure = 1;	/* for 82077 */
static int recalibrate;
static int reset;
static int recover;		/* recalibrate immediately after resetting */
static int seek;

/* BIOS floppy motor timeout counter - FIXME leave this while BIOS driver present */
static unsigned char __far *fl_timeout = (void __far *)0x440L;

/* NOTE: current_DOR tells which motor(s) have been commanded to run,
 * 'running' tells which ones are actually running. The difference is subtle,
 * with typical spinup time of .5 secs.
 */
static unsigned char current_DOR;
static unsigned char running = 0; /* keep track of motors already running */
/*
 * Note that MAX_ERRORS=X doesn't imply that we retry every bad read
 * max X times - some types of errors increase the errorcount by 2 or
 * even 3, so we might actually retry only X/2 times before giving up.
 */
#define MAX_ERRORS 12

/*
 * Maximum number of sectors in a track buffer. Track buffering is disabled
 * if tracks are bigger.
 */
#define MAX_BUFFER_SECTORS 18

/*
 * The 8237 DMA controller cannot access data above 1MB on the origianl PC 
 * (4 bit page register). The AT is different, the page reg is 8 bits while the
 * 2nd DMA controller transfers words only, not bytes and thus up to 128k bytes
 * in one 'swoop'.
 */
#define LAST_DMA_ADDR	(0x100000L - BLOCK_SIZE) /* stick to the 1M limit */

#define _MK_LINADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 4) + (unsigned)(offs)))

/*
 * globals used by 'result()'
 */
#define MAX_REPLIES 7
static unsigned char reply_buffer[MAX_REPLIES];
#define ST0 (reply_buffer[0])
#define ST1 (reply_buffer[1])
#define ST2 (reply_buffer[2])
#define ST3 (reply_buffer[3])

/*
 * Minor number based formats. Each drive type is specified starting
 * from minor number 4 and no auto-probing is used.
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

static struct floppy_struct minor_types[] = {
    {  0,   0, 0,  0, 0, 0x00, 0x00, 0x00, 0x00, NULL},
    { 720,  9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, NULL},	/* 360kB PC diskettes */
    {2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, NULL},	/* 1.2 MB AT-diskettes */
    { 720,  9, 2, 40, 1, 0x2A, 0x02, 0xDF, 0x50, NULL},	/* 360kB in 720kB drive */
    {1440,  9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, NULL},	/* 3.5" 720kB diskette */
    { 720,  9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, NULL},	/* 360kB in 1.2MB drive */
    {1440,  9, 2, 80, 0, 0x23, 0x01, 0xDF, 0x50, NULL},	/* 720kB in 1.2MB drive */
    {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, NULL},	/* 1.44MB diskette */
    {5760, 36, 2, 80, 0, 0x1B, 0x43, 0xAF, 0x28, NULL},	/* 2.88MB diskette */
    /* totSectors/sectors/heads/tracks/stretch/gap/rate/spec1/fmtgap/name */
};

/*
 * Auto-detection. Each drive type has a pair of formats which are
 * used in succession to try to read the disk. If the FDC cannot lock onto
 * the disk, the next format is tried. This uses the variable 'probing'.
 * NOTE: Ignore the duplicates, they're there for a reason.
 */
static struct floppy_struct probe_types[] = {
    { 720,  9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "360k/slow"},	/* 360kB PC 250k DR */
    { 720,  9, 2, 40, 0, 0x2A, 0x01, 0xDF, 0x50, "360k/fast"},	/* 360kB PC 300k DR */
    {2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, "1.2M"},	/* 1.2 MB AT-diskettes */
    { 720,  9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, "360k/1.2M"},	/* 360kB in 1.2MB drive */
    {1440,  9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k"},	/* 3.5" 720kB diskette */
    {1440,  9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k"},	/* 3.5" 720kB diskette */
    {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, "1.44M"},	/* 1.44MB diskette */
    {1440,  9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k/1.2M"},	/* 3.5" 720kB diskette */
    {5760, 36, 2, 80, 0, 0x1B, 0x43, 0xAF, 0x28, "2.88M"},      /* 2.88MB diskette */
    {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, "1.44M"},	/* 1.44MB diskette */
};

/* Auto-detection: Disk type used until the next media change occurs. */
struct floppy_struct *current_type[4];

/* This type is tried first. */
struct floppy_struct *base_type[4];

/*
 * The driver is trying to determine the correct media format
 * while probing is set. rw_interrupt() clears it after a
 * successful access.
 */
static int probing;

/* device reference counters */
static int fd_ref[4];

/* Synchronization of FDC access. */
static int fdc_busy;
static struct wait_queue fdc_wait;

/*
 * Threshold for reporting FDC errors to the console.
 * Setting this to zero may flood your screen when using
 * ultra cheap floppies ;-)
 */
#define MIN_ERRORS      0

/*
 * The block buffer is used for all writes, for formatting and for reads
 * in case track buffering doesn't work or has been turned off.
 */
#define WORD_ALIGNED    __attribute__((aligned(2)))
static char tmp_floppy_area[BLOCK_SIZE] WORD_ALIGNED; /* for now FIXME to be removed */

/*
 * These are global variables, as that's the easiest way to give
 * information to interrupts. They are the data used for the current
 * request.
 */
#define NO_TRACK 255

static int read_track;	        /* set to read entire track */
static int buffer_track = -1;
static int buffer_drive = -1;
static int cur_spec1 = -1;
static int cur_rate = -1;
static struct floppy_struct *floppy;
static unsigned char current_drive = 255;
static unsigned char sector;
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

static void DFPROC delay_loop(int cnt)
{
    while (cnt-- > 0) asm("nop");
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
/*
 * FIXME: The argument (nr) is silently ignored, current_drive being used instead.
 * is this OK?
 */
static void DFPROC floppy_select(unsigned int nr)
{
    DEBUG("sel0x%x-", current_DOR);
    if (current_drive == (current_DOR & 3)) {
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
    current_DOR |= current_drive;
    outb(current_DOR, FD_DOR);

    /* Some FDCs require a delay when changing the current drive */
    del_timer(&select);
    select.tl_expires = jiffies + 2;
    add_timer(&select);
}

static void motor_on_callback(int nr)
{
    DEBUG("mON,");
    clr_irq();	// Experimental
    running |= 0x10 << nr;
    set_irq();
    floppy_select(nr);
}

static struct timer_list motor_on_timer[4] = {
    {NULL, 0, 0, motor_on_callback},
    {NULL, 0, 1, motor_on_callback},
    {NULL, 0, 2, motor_on_callback},
    {NULL, 0, 3, motor_on_callback}
};
static struct timer_list motor_off_timer[4] = {
    {NULL, 0, 0, motor_off_callback},
    {NULL, 0, 1, motor_off_callback},
    {NULL, 0, 2, motor_off_callback},
    {NULL, 0, 3, motor_off_callback}
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
    unsigned char mask = 0x10 << nr;	/* motor on select */

    DEBUG("flpON");
    *fl_timeout = 0; /* Reset BIOS motor timeout counter, neccessary on some machines */
    del_timer(&motor_off_timer[nr]);

    if (mask & running) {
	floppy_select(nr);
	DEBUG("#%x;",current_DOR);
	return;	
    }

    if (!(mask & current_DOR)) {	/* motor not running yet */
	del_timer(&motor_on_timer[nr]);
        /* TEAC 1.44M says 'waiting time' 505ms, may be too little for 5.25in drives. */
	motor_on_timer[nr].tl_expires = jiffies + HZ/2;
	add_timer(&motor_on_timer[nr]);

	current_DOR &= 0xFC;	        /* remove drive select */
	current_DOR |= mask;	        /* set motor select */
	current_DOR |= nr;	        /* set drive select */
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
    motor_off_timer[nr].tl_expires = jiffies + 3 * HZ;
    add_timer(&motor_off_timer[nr]);
    DEBUG("flpOFF-\n");
}

void request_done(int uptodate)
{
    del_timer(&fd_timeout);
    end_request(uptodate);
}

#if CHECK_DISK_CHANGE
/*
 * The check_media_change entry in struct file_operations (fs.h) is not
 * part of the 'normal' setup (only BLOAT_FS), so we're ignoring it for now,
 * assuming the user is smart enough to umount before media changes - or
 * ready for the consequences.
 *
 * floppy-change is never called from an interrupt, so we can relax a bit
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point
 * to the desired drive, but it will probably not survive the sleep if
 * several floppies are used at the same time: thus the loop.
 */
static unsigned int changed_floppies = 0, fake_change = 0;

int floppy_change(struct buffer_head *bh)
{
    unsigned int mask = 1 << (bh->b_dev & 0x03);

    if (MAJOR(bh->b_dev) != MAJOR_NR) {
	printk("floppy_change: not a floppy\n");
	return 0;
    }
    if (fake_change & mask) {
	buffer_track = -1;
	fake_change &= ~mask;
        /* omitting the next line breaks formatting in a horrible way ... */
	changed_floppies &= ~mask;
	return 1;
    }
    if (changed_floppies & mask) {
	buffer_track = -1;
	changed_floppies &= ~mask;
	recalibrate = 1;
	return 1;
    }
    if (!bh)
	return 0;
    if (EBH(bh)->b_dirty)
	ll_rw_blk(WRITE, bh);
    else {
	buffer_track = -1;
	mark_buffer_uptodate(bh, 0);
	ll_rw_blk(READ, bh);
    }
    wait_on_buffer(bh);
    if (changed_floppies & mask) {
	changed_floppies &= ~mask;
	recalibrate = 1;
	return 1;
    }
    return 0;
}
#endif

/* The IBM PC can perform DMA operations by using the DMA chip.  To use it,
 * the DMA (Direct Memory Access) chip is loaded with the 20-bit memory address
 * to be read from or written to, the byte count minus 1, and a read or write
 * opcode.  This routine sets up the DMA chip.  Note that the chip is not
 * capable of doing a DMA across a 64K boundary (e.g., you can't read a
 * 512-byte block starting at physical address 65520).
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
    unsigned int count, physaddr;
    unsigned long dma_addr;
    int use_xms;

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
    use_xms = req->rq_seg >> 16; /* will be nonzero only if XMS configured & XMS buffer */
    physaddr = (req->rq_seg << 4) + (unsigned int)req->rq_buffer;

    count = req->rq_nr_sectors? (unsigned)req->rq_nr_sectors << 9: BLOCK_SIZE;
    if (use_xms || (physaddr + count) < physaddr)
	dma_addr = LAST_DMA_ADDR + 1;	/* force use of bounce buffer */
    else
	dma_addr = _MK_LINADDR(req->rq_seg, req->rq_buffer);

    DEBUG("setupDMA ");

    if (read_track) {	/* mark buffer-track bad, in case all this fails.. */
	buffer_drive = buffer_track = -1;
	count = floppy->sect << 9;	/* sects/trk (one side) times 512 */
	if (floppy->sect & 1 && !head)
	    count += 512; /* add one if head=0 && sector count is odd */
	dma_addr = _MK_LINADDR(DMASEG, 0);
    } else if (dma_addr >= LAST_DMA_ADDR) {
	dma_addr = _MK_LINADDR(kernel_ds, tmp_floppy_area); /* use bounce buffer */
	if (command == FD_WRITE) {
	    xms_fmemcpyw(tmp_floppy_area, kernel_ds, CURRENT->rq_buffer,
                CURRENT->rq_seg, BLOCK_SIZE/2);
	}
    }
    DEBUG("%d/%lx;", count, dma_addr);

    clr_irq();                          /* FIXME unclear interrupts need to be disabled */
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
    printk("fd: can't send to FDC\n");
}

static int DFPROC result(void)
{
    int i = 0, counter, status;

    if (reset)
	return -1;
    for (counter = 0; counter < 10000; counter++) {
	status = inb_p(FD_STATUS) & (STATUS_DIR | STATUS_READY | STATUS_BUSY);
	if (status == STATUS_READY) {	/* done, no more result bytes */
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
    printk("df: getstatus timeout\n");
    return -1;
}

static void DFPROC bad_flp_intr(void)
{
    int errors;

    DEBUG("bad_flpI-");
    current_track = NO_TRACK;
    if (!CURRENT) {
	printk("df: no current request\n");
	reset = recalibrate = 1;
	return;
    } else
	errors = ++CURRENT->rq_errors;
    if (errors > MAX_ERRORS) {
        printk("df: Max retries #%d exceeded\n", errors);
	request_done(0);
    }
    if (errors > MAX_ERRORS / 2)
	reset = 1;
    else
	recalibrate = 1;
}

/* Set perpendicular mode as required, based on data rate, if supported.
 * 1Mbps data rate only possible with 82077-1.
 * TODO: increase MAX_BUFFER_SECTORS.
 */
static void DFPROC perpendicular_mode(unsigned char rate)
{
    if (fdc_version >= FDC_TYPE_82077) {
	output_byte(FD_PERPENDICULAR);
	if (rate & 0x40) {
	    unsigned char r = rate & 0x03;
	    if (r == 0)
		output_byte(2);	/* perpendicular, 500 kbps */
	    else if (r == 3)
		output_byte(3);	/* perpendicular, 1Mbps */
	    else {
		printk("df: Invalid data rate for perpendicular mode!\n");
		reset = 1;
	    }
	} else
	    output_byte(0);	/* conventional mode */
    } else {
	if (rate & 0x40) {
	    printk("df: need 82077 FDC for disc\n");
	    reset = 1;
	}
    }
}				/* perpendicular_mode */

static void DFPROC configure_fdc_mode(void)
{
    if (need_configure && (fdc_version >= FDC_TYPE_82072)) {
	/* Enhanced version with FIFO & write precompensation */
	output_byte(FD_CONFIGURE);
	output_byte(0);
	output_byte(0x1A);	/* FIFO on, polling off, 10 byte threshold */
	output_byte(0);		/* precompensation from track 0 upwards */
	need_configure = 0;
	printk("df: FIFO enabled\n");
    }
    if (cur_spec1 != floppy->spec1) {
	cur_spec1 = floppy->spec1;
	output_byte(FD_SPECIFY);
	output_byte(cur_spec1);	/* hut etc */
	output_byte(6);		/* Head load time =6ms, DMA */
    }
    if (cur_rate != floppy->rate) {
	/* use bit 6 of floppy->rate to indicate perpendicular mode */
	perpendicular_mode(floppy->rate);
	outb_p((cur_rate = (floppy->rate)) & ~0x40, FD_CCR);
    }
}				/* configure_fdc_mode */

static void DFPROC tell_sector(int nr)
{
    if (nr != 7) {
	printk(" -- FDC reply error");
	reset = 1;
    } else
	printk(": track %d, head %d, sector %d", reply_buffer[3],
	       reply_buffer[4], reply_buffer[5]);
}				/* tell_sector */

/*
 * Ok, this interrupt is called after a DMA read/write has succeeded
 * or failed, so we check the results, and copy any buffers.
 * hhb: Added better error reporting.
 */
static void rw_interrupt(void)
{
    unsigned char *buffer_area;
    int nr;
    char bad;

    nr = result();
    /* NOTE: If read_track is active and sector count is uneven, ST0 will
     * always show HD1 as selected at this point. */
    DEBUG("rwI%x|%x|%x-", ST0,ST1,ST2);

    /* check IC to find cause of interrupt */
    switch ((ST0 & ST0_INTR) >> 6) {
    case 1:			/* error occured during command execution */
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
	} else if (CURRENT->rq_errors >= MIN_ERRORS) {
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
	    } else if (ST2 & ST2_WC) {	/* seek error */
		printk("wrong cylinder");
	    } else if (ST2 & ST2_BC) {	/* cylinder marked as bad */
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
    case 2:			/* invalid command given */
	printk("df: Invalid FDC command\n");
	request_done(0);
	return;
    case 3:
	printk("df: Abnormal cmd termination\n");
	bad_flp_intr();
	redo_fd_request();
	return;
    default:			/* (0) Normal command termination */
	break;
    }

    if (probing) {
	int drive = DEVICE_NR(CURRENT->rq_dev);

	printk("df%d: Auto-detected floppy type %s\n", drive, floppy->name);
	current_type[drive] = floppy;
	probing = 0;
    }
    if (read_track) {
        /* This encoding is ugly, should use block-start, block-end instead */
	buffer_track = (seek_track << 1) + head;
	buffer_drive = current_drive;
	buffer_area = (unsigned char *)(sector << 9);
	DEBUG("rd:%04x:%08lx->%04x:%04x;", DMASEG, buffer_area,
		(unsigned long)CURRENT->rq_seg, CURRENT->rq_buffer);
	xms_fmemcpyw(CURRENT->rq_buffer, CURRENT->rq_seg, buffer_area,
            DMASEG, BLOCK_SIZE/2);
    } else if (command == FD_READ 
#ifndef CONFIG_FS_XMS_BUFFER
	   && _MK_LINADDR(CURRENT->rq_seg, CURRENT->rq_buffer) >= LAST_DMA_ADDR
#endif
	                            ) {
	/* if the dest buffer is out of reach for DMA (always the case if using
	 * XMS buffers) we need to read/write via the bounce buffer */
        xms_fmemcpyw(CURRENT->rq_buffer, CURRENT->rq_seg, tmp_floppy_area,
            kernel_ds, BLOCK_SIZE/2);
        printk("fd: illegal buffer usage, rq_buffer %04x:%04x\n",
            CURRENT->rq_seg, CURRENT->rq_buffer);
    }
    request_done(1);
    //printk("RQOK;");
    redo_fd_request();	/* Continue with the next request - if any */
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
    setup_DMA();
    do_floppy = rw_interrupt;
    output_byte(command);
    output_byte(head << 2 | current_drive);
    output_byte(track);
    output_byte(head);
    if (read_track)
        output_byte(1);	        /* always start at 1 */
    else
        output_byte(sector+1);
    output_byte(2);		/* sector size = 512 */
    output_byte(floppy->sect);
    output_byte(floppy->gap);
    output_byte(0xFF);	/* sector size, 0xff unless sector size==0 (128b) */
    if (reset)
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
	printk("df%d: seek failed\n", current_drive);
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
#ifdef CONFIG_TRACK_CACHE
    read_track = (command == FD_READ) && (CURRENT->rq_errors < 4) &&
	(floppy->sect <= MAX_BUFFER_SECTORS);
#endif
    DEBUG("trns%d-", read_track);

    configure_fdc_mode();	/* FIXME: Why are we doing this here??? 
    				 * should be done once per media change ... */

    if (reset) {
	redo_fd_request();
	return;
    }
    if (!seek) {
	setup_rw_floppy();
	return;
    }

    /* OK; need to change tracks ... */
    do_floppy = seek_interrupt;
    DEBUG("sk%d;",seek_track);
    output_byte(FD_SEEK);
    output_byte((head << 2) | current_drive);
    output_byte(seek_track);
    if (reset)	                /* error in output_byte() */
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

    /* FIXME this may not make sense: We're waiting for recal_interrupt
     * why redo_fd_request here when recal_interrupt is doing it ???
     * 'reset' gets set in recal_interrupt, maybe that's it ???  */
    if (reset)
	redo_fd_request();
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
    DEBUG("recalI-%x", ST0);	/* Should be 0x20, Seek End */
    /* Recalibrate until track 0 is reached. Might help on some errors. */
    if (ST0 & 0x10) {           /* Look for UnitCheck, which will happen regularly
    				 * on 80 track drives because RECAL only steps 77 times */
	recalibrate_floppy();	/* FIXME: may loop, should limit nr of recalibrates */
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
	(void) result();
    }
    output_byte(FD_SPECIFY);
    output_byte(cur_spec1);	/* hut etc */
    output_byte(6);		/* Head load time =6ms, DMA */
    configure_fdc_mode();	/* reprogram fdc */
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
    need_configure = 1;
    if (!initial_reset_flag)
	printk("df: reset_floppy called\n");
    clr_irq();              /* FIXME don't busyloop with interrupts off, use timer */
    outb_p(current_DOR & ~0x04, FD_DOR);
    delay_loop(1000);
    outb(current_DOR, FD_DOR);
    set_irq();
}

static void floppy_shutdown(void)
{
    DEBUG("[%u]shtdwn0x%x|%x-", (unsigned int)jiffies, current_DOR, running);
    do_floppy = NULL;
    printk("df%d: FDC cmd timeout\n", current_drive);
    request_done(0);
    recover = 1;
    reset_floppy();
    redo_fd_request();
}

#if CLEAR_DIR_REG
static void shake_done(void)
{
    /* Need SENSEI to clear the interrupt per spec, required by QEMU, not by
     * real hardware	*/
    output_byte(FD_SENSEI);	/* TESTING FIXME */
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

/*
 * The result byte after the SENSEI cmd is ST3, not ST0
 */
static int DFPROC retry_recal(void (*proc)())
{
    DEBUG("rrecal-");
    output_byte(FD_SENSEI);
    if (result() == 2 && (ST0 & 0x10) != 0x10) /* track 0 test */
	return 0;		/* No 'unit check': We're OK */
    do_floppy = proc;		/* otherwise recalibrate */
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
	return;	                /* just return if retry_recal initiated a RECAL */
    do_floppy = shake_done;
    output_byte(FD_SEEK);
    output_byte(head << 2 | current_drive);
    output_byte(1);
}

/*
 * (User-provided) media information is _not_ discarded after a media change
 * if the corresponding keep_data flag is non-zero. Positive values are
 * decremented after each probe.
 */
static int keep_data[4];
#endif /* CLEAR_DIR_REG */

static void DFPROC floppy_ready(void)
{
    DEBUG("RDY0x%x,%d,%d-", inb(FD_DIR), reset, recalibrate);

#if CLEAR_DIR_REG
    /* check if disk changed since last cmd (PC/AT+) */
    if (fdc_version >= FDC_TYPE_8272PC_AT && (inb(FD_DIR) & 0x80)) {

#if CHECK_DISK_CHANGE
	changed_floppies |= 1 << current_drive;
#endif
	buffer_track = -1;
	if (keep_data[current_drive]) {
	    if (keep_data[current_drive] > 0)
		keep_data[current_drive]--;
	} else {
		/* FIXME: this is non sensical: Need to assume that a medium change
		 * means a new medium of the same format as the prev until it fails.
		 */
	    if (current_type[current_drive] != NULL)
		printk("df%d: Disk type undefined after disk change\n", current_drive);
	    current_type[current_drive] = NULL;
	}
	/* Forcing the drive to seek makes the "media changed" condition go
	 * away. There should be a cleaner solution for that ...
	 * FIXME: This is way too slow and recal is not seek ...
	 */
	if (!reset && !recalibrate) {
	    if (current_track && current_track != NO_TRACK)
		do_floppy = shake_zero;
	    else
		do_floppy = shake_one;
	    output_byte(FD_RECALIBRATE);
	    output_byte(current_drive);
	    return;
	}
    }
#endif

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
    unsigned int start;
    struct request *req;
    int device, drive;
    unsigned int tmp;

  repeat:
    req = CURRENT;
    if (!req) {
        if (!fdc_busy)
            printk("FDC access conflict!");
        fdc_busy = 0;
        wake_up(&fdc_wait);
        do_floppy = NULL;
        return;
    }
    CHECK_REQUEST(req);

    seek = 0;
    probing = 0;
    device = MINOR(req->rq_dev) >> MINOR_SHIFT;
    drive = device & 3;
    if (device > 3)
	floppy = (device >> 2) + minor_types;
    else {			/* Auto-detection */
	floppy = current_type[drive];
	if (!floppy) {
	    probing = 1;
	    floppy = base_type[drive];
	    if (!floppy) {
                printk("df%d: no base drive type, aborting\n", drive);
		request_done(0);
		goto repeat;
	    }
            if (!recalibrate) {
	        if (req->rq_errors & 1)
		    floppy++;
                printk("df%d: auto-probe #%d %s (%d)\n", drive, req->rq_errors,
                    floppy->name, floppy-probe_types);
            }
        }
    }
    DEBUG("[%u]redo-%c %d(%s) bl %u;", (unsigned int)jiffies, 
	req->rq_cmd == WRITE? 'W':'R', device, floppy->name, req->rq_sector);
    DEBUG("df%d: %c sector %ld\n", DEVICE_NR(req->rq_dev),

    req->rq_cmd==WRITE? 'W' : 'R', req->rq_sector);
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
    DEBUG("%d:%d:%d:%d; ", start, sector, head, track);

    /* timer for hung operations, 6 secs probably too long ... */
    del_timer(&fd_timeout);
    fd_timeout.tl_expires = jiffies + 6 * HZ;
    add_timer(&fd_timeout);

    DEBUG("prep %d|%d,%d|%d-", buffer_track, seek_track, buffer_drive, current_drive);

    if ((((seek_track << 1) + head) == buffer_track) && (current_drive == buffer_drive)) {
	/* Requested block is in the buffer. If reading, go get it.
	 * If the sector count is odd, we buffer sectors+1 when head=0 to get an even
	 * number of sectors (full blocks). When head=1 we read the entire track
	 * and ignore the first sector.
	 */
	DEBUG("bufrd chs %d/%d/%d\n", seek_track, head, sector);
	char *buf_ptr = (char *) (sector << 9);
	if (command == FD_READ) {	/* requested data is in buffer */
	    xms_fmemcpyw(req->rq_buffer, req->rq_seg, buf_ptr, DMASEG, BLOCK_SIZE/2);
	    request_done(1);
	    goto repeat;
    	} else if (command == FD_WRITE)	/* update track buffer */
	    xms_fmemcpyw(buf_ptr, DMASEG, req->rq_buffer, req->rq_seg, BLOCK_SIZE/2);
    } 

    if (seek_track != current_track)
	seek = 1;

    floppy_on(current_drive);
}

void do_fd_request(void)
{
    DEBUG("fdrq:");
    while (fdc_busy)
	sleep_on(&fdc_wait);
    fdc_busy = 1;
    redo_fd_request();
}

static int fd_ioctl(struct inode *inode,
		    struct file *filp, unsigned int cmd, unsigned int param)
{
/* FIXME: Get this back in when everything else is working */
    struct floppy_struct *this_floppy;
    int drive, err = -EINVAL;
    struct hd_geometry *loc = (struct hd_geometry *) param;

    if (!inode || !inode->i_rdev)
	return -EINVAL;
    drive = MINOR(inode->i_rdev) >> MINOR_SHIFT;
    if (drive > 3)
	this_floppy = &minor_types[drive >> 2];
    else if ((this_floppy = current_type[drive & 3]) == NULL)
	return -ENODEV;

    switch (cmd) {
    case HDIO_GETGEO:	/* need this one for the sys/makeboot command */
    case FDGETPRM:
	err = verify_area(VERIFY_WRITE, (void *) param, sizeof(struct hd_geometry));
	if (!err) {
	    put_user_char(this_floppy->head, &loc->heads);
	    put_user_char(this_floppy->sect, &loc->sectors);
	    put_user(this_floppy->track, &loc->cylinders);
	    put_user_long(0L, &loc->start);
	}
	return err;
#ifdef UNUSED
    case FDFMTBEG:
	if (!suser())
	    return -EPERM;
	return 0;
    case FDFMTEND:
	if (!suser())
	    return -EPERM;
	clr_irq();
	fake_change |= 1 << (drive & 3);
	set_irq();
	drive &= 3;
	cmd = FDCLRPRM;
	break;
    case FDFMTTRK:
	if (!suser())
	    return -EPERM;
	if (fd_ref[drive & 3] != 1)
	    return -EBUSY;
	clr_irq();
	while (format_status != FORMAT_NONE)
	    sleep_on(&format_done);
	memcpy_fromfs((char *)(&format_req),
		    (char *)param, sizeof(struct format_descr));
	format_req.device = drive; 	/* Should this be a full device ID? */
	format_status = FORMAT_WAIT;
	format_errors = 0;
	while (format_status != FORMAT_OKAY && format_status != FORMAT_ERROR) {
	    if (fdc_busy)
		sleep_on(&fdc_wait);
	    else {
		fdc_busy = 1;
		redo_fd_request();
	    }
	}
	while (format_status != FORMAT_OKAY && format_status != FORMAT_ERROR)
	    sleep_on(&format_done);
	set_irq();
	err = format_status == FORMAT_OKAY;
	format_status = FORMAT_NONE;
	floppy_off(drive & 3);
	wake_up(&format_done);
	return err ? 0 : -EIO;
    case FDFLUSH:
	if (!permission(inode, 2))
	    return -EPERM;
	clr_irq();
	fake_change |= 1 << (drive & 3);
	set_irq();
	check_disk_change(inode->i_rdev);
	return 0;
    }
    if (!suser())
	return -EPERM;
    if (drive < 0 || drive > 3)
	return -EINVAL;
    switch (cmd) {
    case FDCLRPRM:
	current_type[drive] = NULL;
	keep_data[drive] = 0;
	break;
    case FDSETPRM:
    case FDDEFPRM:
	memcpy_fromfs(user_params + drive,
		      (void *) param, sizeof(struct floppy_struct));
	current_type[drive] = &user_params[drive];
	if (cmd == FDDEFPRM)
	    keep_data[drive] = -1;
	else {
	    clr_irq();
	    while (fdc_busy)
		sleep_on(&fdc_wait);
	    fdc_busy = 1;
	    set_irq();
	    outb_p((current_DOR & 0xfc) | drive | (0x10 << drive), FD_DOR);
	    delay_loop(1000);
	    if (inb(FD_DIR) & 0x80)
		keep_data[drive] = 1;
	    else
		keep_data[drive] = 0;
	    outb_p(current_DOR, FD_DOR);
	    fdc_busy = 0;
	    wake_up(&fdc_wait);
	}
	break;
#endif
    default:
	return -EINVAL;
    }
    return err;
}

static int INITPROC CMOS_READ(int addr)
{
    outb_p(addr, 0x70);
    return inb_p(0x71);
}

static struct floppy_struct * INITPROC find_base(int drive, int code)
{
    struct floppy_struct *base;

    if (code > 0 && code < 6) {
	base = &probe_types[(code - 1) * 2];
	printk("df%d is %s (%d)", drive, base->name, code);
	return base;
    }
    printk("df%d is unknown (%d)", drive, code);
    return NULL;
}

static void INITPROC config_types(void)
{
    printk("df: CMOS ");
    base_type[0] = find_base(0, (CMOS_READ(0x10) >> 4) & 0xF);
    //base_type[0] = find_base(0, 1);     /* force 360k FIXME add setup table */
    if (((CMOS_READ(0x14) >> 6) & 1) != 0) {
	printk(", ");
	base_type[1] = find_base(1, CMOS_READ(0x10) & 0xF);
    }
    printk("\n");
}

static int floppy_open(struct inode *inode, struct file *filp)
{
    int drive, dev, err;

    drive = MINOR(inode->i_rdev) >> MINOR_SHIFT;
    dev = drive & 3;

#if CHECK_DISK_CHANGE
    if (filp && filp->f_mode)
	check_disk_change(inode->i_rdev);
#endif

    probing = 0;
    if (drive > 3)		/* forced floppy type */
	floppy = (drive >> 2) + minor_types;
    else {			/* Auto-detection */
	floppy = current_type[dev];
	if (!floppy) {
	    probing = 1;
	    floppy = base_type[dev];
	    if (!floppy)
		return -ENXIO;
	}
    }

    if (fd_ref[dev] == 0) {
        err = floppy_register();
        if (err) return err;
        buffer_drive = buffer_track = -1;
    }

    fd_ref[dev]++;
    inode->i_size = ((sector_t)floppy->size) << 9;  /* NOTE: assumes sector size 512 */
    DEBUG("df%d: open dv %x, sz %lu, %s\n", drive, inode->i_rdev, inode->i_size,
        floppy->name);

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
        floppy_deregister();
    }
}

static struct file_operations floppy_fops = {
    NULL,			/* lseek - default */
    block_read,			/* read - general block-dev read */
    block_write,		/* write - general block-dev write */
    NULL,			/* readdir - bad */
    NULL,			/* select */
    fd_ioctl,			/* ioctl */
    floppy_open,		/* open */
    floppy_release,		/* release */
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
    do_floppy = NULL;		/* ignore only once */
}

static void floppy_interrupt(int irq, struct pt_regs *regs)
{
    void (*handler) () = do_floppy;

    do_floppy = NULL;
    if (!handler)
	handler = unexpected_floppy_interrupt;
    //printk("$");
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
    output_byte(FD_VERSION);	/* get FDC version code */
    if (result() != 1) {
        printk("df: can't get FDC version\n");
        return 0;
    }
    switch (reply_buffer[0]) {
    case 0x80:
        if (SETUP_CPU_TYPE > 5) {       /* 80286 CPU PC/AT or better */
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
    /* testing FIXME --  the FDC has just been reset by the BIOS, do we need this? */
    if (type < FDC_TYPE_82077) {
        initial_reset_flag = 1;
        reset_floppy();
    }
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
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &floppy_fops)) {
	printk("df: init error\n");
	return;
    }
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
    config_types();
}
