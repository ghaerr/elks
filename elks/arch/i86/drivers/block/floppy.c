/* linux/kernel/floppy.c
 *
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
 *
 * removed volatile and fixed function definitions
 */

#define TRP_TIMER
#define TRP_SIG

#define REALLY_SLOW_IO
#define FLOPPY_IRQ 6
#define FLOPPY_DMA 2

#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
/*#include <linuxmt/timer.h>*/
#include <linuxmt/mm.h>
#include <linuxmt/signal.h>
#include <linuxmt/fdreg.h>
#include <linuxmt/fd.h>
#include <linuxmt/errno.h>
#include <linuxmt/config.h>

#include <arch/dma.h>
#include <arch/system.h>
#include <arch/io.h>
#include <arch/segment.h>

#ifdef CONFIG_BLK_DEV_FD
#if 1
/* Hack until this code works */
void floppy_init(void)
{
/* Do nothing */
}

#else

#define FLOPPYDISK
#define MAJOR_NR FLOPPY_MAJOR
#include "blk.h"

static unsigned int changed_floppies = 0, fake_change = 0;

static int initial_reset_flag = 0;
static int need_configure = 1;	/* for 82077 */
static int recalibrate = 0;
static int reset = 0;
static int recover = 0;		/* recalibrate immediately after resetting */
static int seek = 0;

static unsigned char current_DOR = 0x0C;
static unsigned char running = 0;

#define TYPE(x) ((x)>>2)
#define DRIVE(x) ((x)&0x03)

/*
 * Note that MAX_ERRORS=X doesn't imply that we retry every bad read
 * max X times - some types of errors increase the errorcount by 2 or
 * even 3, so we might actually retry only X/2 times before giving up.
 */
#define MAX_ERRORS 12

/*
 * Maximum disk size (in kilobytes). This default is used whenever the
 * current disk size is unknown.
 */
#define MAX_DISK_SIZE 1440

/*
 * Maximum number of sectors in a track buffer. Track buffering is disabled
 * if tracks are bigger.
 */
#define MAX_BUFFER_SECTORS 18

/*
 * The DMA channel used by the floppy controller cannot access data at
 * addresses >= 16MB
 *
 * Went back to the 1MB limit, as some people had problems with the floppy
 * driver otherwise. It doesn't matter much for performance anyway, as most
 * floppy accesses go through the track buffer.
 */
#define LAST_DMA_ADDR	(0x100000 - BLOCK_SIZE)

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
 * This struct defines the different floppy types.
 *
 * The 'stretch' tells if the tracks need to be doubled for some
 * types (ie 360kB diskette in 1.2MB drive etc). Others should
 * be self-explanatory.
 */
static struct floppy_struct floppy_type[] = {
    {0, 0, 0, 0, 0, 0x00, 0x00, 0x00, 0x00, NULL},	/* no testing */
    {720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, NULL},	/* 360kB PC diskettes */
    {2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, NULL},	/* 1.2 MB AT-diskettes */
    {720, 9, 2, 40, 1, 0x2A, 0x02, 0xDF, 0x50, NULL},	/* 360kB in 720kB drive */
    {1440, 9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, NULL},	/* 3.5" 720kB diskette */
    {720, 9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, NULL},	/* 360kB in 1.2MB drive */
    {1440, 9, 2, 80, 0, 0x23, 0x01, 0xDF, 0x50, NULL},	/* 720kB in 1.2MB drive */
    {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, NULL},	/* 1.44MB diskette */
};

/*
 * Auto-detection. Each drive type has a pair of formats which are
 * used in succession to try to read the disk. If the FDC cannot lock onto
 * the disk, the next format is tried. This uses the variable 'probing'.
 */
static struct floppy_struct floppy_types[] = {
    {720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "360k/PC"},	/* 360kB PC diskettes */
    {720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "360k/PC"},	/* 360kB PC diskettes */
    {2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, "1.2M"},	/* 1.2 MB AT-diskettes */
    {720, 9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, "360k/AT"},	/* 360kB in 1.2MB drive */
    {1440, 9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k"},	/* 3.5" 720kB diskette */
    {1440, 9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k"},	/* 3.5" 720kB diskette */
    {2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, "1.44M"},	/* 1.44MB diskette */
    {1440, 9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "720k/AT"},	/* 3.5" 720kB diskette */
};

/* Auto-detection: Disk type used until the next media change occurs. */
struct floppy_struct *current_type[4] = { NULL, NULL, NULL, NULL };

/* This type is tried first. */
struct floppy_struct *base_type[4];

/*
 * User-provided type information. current_type points to
 * the respective entry of this array.
 */
struct floppy_struct user_params[4];

static int floppy_sizes[] = {
    MAX_DISK_SIZE, MAX_DISK_SIZE, MAX_DISK_SIZE, MAX_DISK_SIZE,
    360, 360, 360, 360,
    1200, 1200, 1200, 1200,
    360, 360, 360, 360,
    720, 720, 720, 720,
    360, 360, 360, 360,
    720, 720, 720, 720,
    1440, 1440, 1440, 1440
};

/*
 * The driver is trying to determine the correct media format
 * while probing is set. rw_interrupt() clears it after a
 * successful access.
 */
static int probing = 0;

/*
 * (User-provided) media information is _not_ discarded after a media change
 * if the corresponding keep_data flag is non-zero. Positive values are
 * decremented after each probe.
 */
static int keep_data[4] = { 0, 0, 0, 0 };

/*
 * Announce successful media type detection and media information loss after
 * disk changes.
 * Also used to enable/disable printing of overrun warnings.
 */
static ftd_msg[4] = { 0, 0, 0, 0 };

/* Prevent "aliased" accesses. */

static fd_ref[4] = { 0, 0, 0, 0 };
static fd_device[4] = { 0, 0, 0, 0 };

/* Synchronization of FDC access. */
static int format_status = FORMAT_NONE, fdc_busy = 0;
static struct wait_queue fdc_wait, format_done;

/* Errors during formatting are counted here. */
static int format_errors;

/* Format request descriptor. */
static struct format_descr format_req;

/*
 * Current device number. Taken either from the block header or from the
 * format request descriptor.
 */
#define CURRENT_DEVICE (format_status == FORMAT_BUSY ? format_req.device : \
   (CURRENT->dev))

/* Current error count. */
#define CURRENT_ERRORS (format_status == FORMAT_BUSY ? format_errors : \
    (CURRENT->errors))

/*
 * Threshold for reporting FDC errors to the console.
 * Setting this to zero may flood your screen when using
 * ultra cheap floppies ;-)
 */
static unsigned short min_report_error_cnt[4] = { 2, 2, 2, 2 };

/*
 * Rate is 0 for 500kb/s, 1 for 300kbps, 2 for 250kbps
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),
 * H is head unload time (1=16ms, 2=32ms, etc)
 *
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).
 */

/*
 * Track buffer and block buffer (in case track buffering doesn't work).
 * Because these are written to by the DMA controller, they must
 * not contain a 64k byte boundary crossing, or data will be
 * corrupted/lost. Alignment of these is enforced in boot/head.s.
 * Note that you must not change the sizes below without updating head.s.
 */
extern char tmp_floppy_area[BLOCK_SIZE];
extern char floppy_track_buffer[512 * 2 * MAX_BUFFER_SECTORS];

static void redo_fd_request();

/*
 * These are global variables, as that's the easiest way to give
 * information to interrupts. They are the data used for the current
 * request.
 */
#define NO_TRACK 255

static int read_track = 0;	/* flag to indicate if we want to read entire track */
static int buffer_track = -1;
static int buffer_drive = -1;
static int cur_spec1 = -1;
static int cur_rate = -1;
static struct floppy_struct *floppy = floppy_type;
static unsigned char current_drive = 255;
static unsigned char sector = 0;
static unsigned char head = 0;
static unsigned char track = 0;
static unsigned char seek_track = 0;
static unsigned char current_track = NO_TRACK;
static unsigned char command = 0;
static unsigned char fdc_version = FDC_TYPE_STD;	/* FDC version code */

static void floppy_ready();

#ifdef TRP_ASM
#define copy_buffer(from,to) \
__asm__("cld ; rep ; movsl" \
	: \
	:"c" (BLOCK_SIZE/4),"S" ((long)(from)),"D" ((long)(to)) \
	:"cx","di","si")
#else
static void copy_buffer(from, to)
{
    memcpy(to, from, BLOCK_SIZE);
}
#endif

static void select_callback(unsigned long unused)
{
    floppy_ready();
}

static void floppy_select(unsigned int nr)
{
#ifndef TRP_TIMER
    static struct timer_list select = { NULL, NULL, 0, 0, select_callback };
#endif

    if (current_drive == (current_DOR & 3)) {
	floppy_ready();
	return;
    }
    seek = 1;
    current_track = NO_TRACK;
    current_DOR &= 0xFC;
    current_DOR |= current_drive;
    outb(current_DOR, FD_DOR);

#ifndef TRP_TIMER
    del_timer(&select);
    select.expires = 2;
    add_timer(&select);
#endif
}

static void motor_on_callback(unsigned long nr)
{
    running |= 0x10 << nr;
    floppy_select(nr);
}

#ifndef TRP_TIMER
static struct timer_list motor_on_timer[4] = {
    {NULL, NULL, 0, 0, motor_on_callback},
    {NULL, NULL, 0, 1, motor_on_callback},
    {NULL, NULL, 0, 2, motor_on_callback},
    {NULL, NULL, 0, 3, motor_on_callback}
};
#endif

static void motor_off_callback(unsigned long nr)
{
    unsigned char mask = ~(0x10 << nr);
    cli();
    running &= mask;
    current_DOR &= mask;
    outb(current_DOR, FD_DOR);
    sti();
}

#ifndef TRP_TIMER
static struct timer_list motor_off_timer[4] = {
    {NULL, NULL, 0, 0, motor_off_callback},
    {NULL, NULL, 0, 1, motor_off_callback},
    {NULL, NULL, 0, 2, motor_off_callback},
    {NULL, NULL, 0, 3, motor_off_callback}
};
#endif

static void floppy_on(unsigned int nr)
{
    unsigned char mask = 0x10 << nr;

#ifndef TRP_TIMER
    del_timer(motor_off_timer + nr);
#endif

    if (mask & running)
	floppy_select(nr);
#ifndef TRP_TIMER
    if (!(mask & current_DOR)) {
	del_timer(motor_on_timer + nr);
	motor_on_timer[nr].expires = HZ;
	add_timer(motor_on_timer + nr);
    }
#endif
    current_DOR &= 0xFC;
    current_DOR |= mask;
    current_DOR |= nr;
    outb(current_DOR, FD_DOR);
}

static void floppy_off(unsigned int nr)
{
#ifndef TRP_TIMER
    del_timer(motor_off_timer + nr);
    motor_off_timer[nr].expires = 3 * HZ;
    add_timer(motor_off_timer + nr);
#endif
}

void request_done(int uptodate)
{
#ifndef TRP_TIMER
    timer_active &= ~(1 << FLOPPY_TIMER);
#endif

    if (format_status != FORMAT_BUSY)
	end_request(uptodate);
    else {
	format_status = uptodate ? FORMAT_OKAY : FORMAT_ERROR;
	wake_up(&format_done);
    }
}

/*
 * floppy-change is never called from an interrupt, so we can relax a bit
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point
 * to the desired drive, but it will probably not survive the sleep if
 * several floppies are used at the same time: thus the loop.
 */
int floppy_change(struct buffer_head *bh)
{
    unsigned int mask = 1 << (bh->b_dev & 0x03);

    if (MAJOR(bh->b_dev) != MAJOR_NR) {
	printk("floppy_changed: not a floppy\n");
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
    if (buffer_dirty(bh))
	ll_rw_block(WRITE, 1, &bh);
    else {
	buffer_track = -1;
	mark_buffer_uptodate(bh, 0);
	ll_rw_block(READ, 1, &bh);
    }
    wait_on_buffer(bh);
    if (changed_floppies & mask) {
	changed_floppies &= ~mask;
	recalibrate = 1;
	return 1;
    }
    return 0;
}

static void setup_DMA(void)
{
    unsigned long addr, count;
    unsigned char dma_code;

    dma_code = DMA_WRITE;
    if (command == FD_READ)
	dma_code = DMA_READ;
    if (command == FD_FORMAT) {
	addr = (long) tmp_floppy_area;
	count = floppy->sect * 4;
    } else {
	addr = (long) CURRENT->buffer;
	count = 1024;
    }
    if (read_track) {
/* mark buffer-track bad, in case all this fails.. */
	buffer_drive = buffer_track = -1;
	count = floppy->sect * floppy->head * 512;
	addr = (long) floppy_track_buffer;
    } else if (addr >= LAST_DMA_ADDR) {
	addr = (long) tmp_floppy_area;
	if (command == FD_WRITE)
	    copy_buffer(CURRENT->buffer, tmp_floppy_area);
    }
    cli();
    disable_dma(FLOPPY_DMA);
    clear_dma_ff(FLOPPY_DMA);
    set_dma_mode(FLOPPY_DMA,
		 (command == FD_READ) ? DMA_MODE_READ : DMA_MODE_WRITE);
    set_dma_addr(FLOPPY_DMA, addr);
    set_dma_count(FLOPPY_DMA, count);
    enable_dma(FLOPPY_DMA);
    sti();
}

static void output_byte(char byte)
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
    printk("Unable to send byte to FDC\n");
}

static int result(void)
{
    int i = 0, counter, status;

    if (reset)
	return -1;
    for (counter = 0; counter < 10000; counter++) {
	status = inb_p(FD_STATUS) & (STATUS_DIR | STATUS_READY | STATUS_BUSY);
	if (status == STATUS_READY) {
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
    printk("Getstatus times out\n");
    return -1;
}

static void bad_flp_intr(void)
{
    int errors;

    current_track = NO_TRACK;
    if (format_status == FORMAT_BUSY)
	errors = ++format_errors;
    else if (!CURRENT) {
	printk("%s: no current request\n", DEVICE_NAME);
	reset = recalibrate = 1;
	return;
    } else
	errors = ++CURRENT->errors;
    if (errors > MAX_ERRORS) {
	request_done(0);
    }
    if (errors > MAX_ERRORS / 2)
	reset = 1;
    else
	recalibrate = 1;
}

/* Set perpendicular mode as required, based on data rate, if supported.
 * 82077 Untested! 1Mbps data rate only possible with 82077-1.
 * TODO: increase MAX_BUFFER_SECTORS, add floppy_type entries.
 */
static void perpendicular_mode(unsigned char rate)
{
    if (fdc_version == FDC_TYPE_82077) {
	output_byte(FD_PERPENDICULAR);
	if (rate & 0x40) {
	    unsigned char r = rate & 0x03;
	    if (r == 0)
		output_byte(2);	/* perpendicular, 500 kbps */
	    else if (r == 3)
		output_byte(3);	/* perpendicular, 1Mbps */
	    else {
		printk("%s: Invalid data rate for perpendicular mode!\n",
		       DEVICE_NAME);
		reset = 1;
	    }
	} else
	    output_byte(0);	/* conventional mode */
    } else {
	if (rate & 0x40) {
	    printk("%s: perpendicular mode not supported by this FDC.\n",
		   DEVICE_NAME);
	    reset = 1;
	}
    }
}				/* perpendicular_mode */

/*
 * This has only been tested for the case fdc_version == FDC_TYPE_STD.
 * In case you have a 82077 and want to test it, you'll have to compile
 * with `FDC_FIFO_UNTESTED' defined. You may also want to add support for
 * recognizing drives with vertical recording support.
 */
static void configure_fdc_mode(void)
{
    if (need_configure && (fdc_version == FDC_TYPE_82077)) {
	/* Enhanced version with FIFO & vertical recording. */
	output_byte(FD_CONFIGURE);
	output_byte(0);
	output_byte(0x1A);	/* FIFO on, polling off, 10 byte threshold */
	output_byte(0);		/* precompensation from track 0 upwards */
	need_configure = 0;
	printk("%s: FIFO enabled\n", DEVICE_NAME);
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
	outb_p((cur_rate = (floppy->rate)) & ~0x40, FD_DCR);
    }
}				/* configure_fdc_mode */

static void tell_sector(int nr)
{
    if (nr != 7) {
	printk(" -- FDC reply errror");
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
    char *buffer_area;
    int nr;
    char bad;

    nr = result();
    /* check IC to find cause of interrupt */
    switch ((ST0 & ST0_INTR) >> 6) {
    case 1:			/* error occured during command execution */
	bad = 1;
	if (ST1 & ST1_WP) {
	    printk("%s: Drive %d is write protected\n", DEVICE_NAME,
		   current_drive);
	    request_done(0);
	    bad = 0;
	} else if (ST1 & ST1_OR) {
	    if (ftd_msg[ST0 & ST0_DS])
		printk("%s: Over/Underrun - retrying\n", DEVICE_NAME);
	    /* could continue from where we stopped, but ... */
	    bad = 0;
	} else if (CURRENT_ERRORS > min_report_error_cnt[ST0 & ST0_DS]) {
	    printk("%s: %d: ", DEVICE_NAME, ST0 & ST0_DS);
	    if (ST0 & ST0_ECE) {
		printk("Recalibrate failed!");
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
		    printk("probe failed...");
	    } else if (ST2 & ST2_WC) {	/* seek error */
		printk("wrong cylinder");
	    } else if (ST2 & ST2_BC) {	/* cylinder marked as bad */
		printk("bad cylinder");
	    } else {
		printk("unknown error. ST[0..3] are: 0x%x 0x%x 0x%x 0x%x\n",
		       ST0, ST1, ST2, ST3);
	    }
	    printk("\n");

	}
	if (bad)
	    bad_flp_intr();
	redo_fd_request();
	return;
    case 2:			/* invalid command given */
	printk("%s: Invalid FDC command given!\n", DEVICE_NAME);
	request_done(0);
	return;
    case 3:
	printk("%s: Abnormal termination caused by polling\n", DEVICE_NAME);
	bad_flp_intr();
	redo_fd_request();
	return;
    default:			/* (0) Normal command termination */
	break;
    }

    if (probing) {
	int drive = MINOR(CURRENT->dev);

	if (ftd_msg[drive])
	    printk("Auto-detected floppy type %s in fd%d\n",
		   floppy->name, drive);
	current_type[drive] = floppy;
	floppy_sizes[drive] = floppy->size >> 1;
	probing = 0;
    }
    if (read_track) {
	buffer_track = seek_track;
	buffer_drive = current_drive;
	buffer_area = floppy_track_buffer +
	    ((sector - 1 + head * floppy->sect) << 9);
	copy_buffer(buffer_area, CURRENT->buffer);
    } else if (command == FD_READ &&
	       (unsigned long) (CURRENT->buffer) >= LAST_DMA_ADDR)
	    copy_buffer(tmp_floppy_area, CURRENT->buffer);
    request_done(1);
    redo_fd_request();
}

/*
 * We try to read tracks, but if we get too many errors, we go back to
 * reading just one sector at a time. This means we should be able to
 * read a sector even if there are other bad sectors on this track.
 */
void setup_rw_floppy(void)
{
    setup_DMA();
    do_floppy = rw_interrupt;
    output_byte(command);
    if (command != FD_FORMAT) {
	if (read_track) {
	    output_byte(current_drive);
	    output_byte(track);
	    output_byte(0);
	    output_byte(1);
	} else {
	    output_byte(head << 2 | current_drive);
	    output_byte(track);
	    output_byte(head);
	    output_byte(sector);
	}
	output_byte(2);		/* sector size = 512 */
	output_byte(floppy->sect);
	output_byte(floppy->gap);
	output_byte(0xFF);	/* sector size (0xff when n!=0 ?) */
    } else {
	output_byte(head << 2 | current_drive);
	output_byte(2);
	output_byte(floppy->sect);
	output_byte(floppy->fmt_gap);
	output_byte(FD_FILL_BYTE);
    }
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
    output_byte(FD_SENSEI);
    if (result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track) {
	printk("%s: seek failed\n", DEVICE_NAME);
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
 * selected).
 */
static void transfer(void)
{
    read_track = (command == FD_READ) && (CURRENT_ERRORS < 4) &&
	(floppy->sect <= MAX_BUFFER_SECTORS);

    configure_fdc_mode();

    if (reset) {
	redo_fd_request();
	return;
    }
    if (!seek) {
	setup_rw_floppy();
	return;
    }

    do_floppy = seek_interrupt;
    output_byte(FD_SEEK);
    if (read_track)
	output_byte(current_drive);
    else
	output_byte((head << 2) | current_drive);
    output_byte(seek_track);
    if (reset)
	redo_fd_request();
}

static void recalibrate_floppy()
{
    recalibrate = 0;
    current_track = 0;
    do_floppy = recal_interrupt;
    output_byte(FD_RECALIBRATE);
    output_byte(head << 2 | current_drive);
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
    if (result() != 2 || (ST0 & 0xE0) == 0x60)
	reset = 1;
    /* Recalibrate until track 0 is reached. Might help on some errors. */
    if ((ST0 & 0x10) == 0x10)
	recalibrate_floppy();	/* FIXME: should limit nr of recalibrates */
    else
	redo_fd_request();
}

static void unexpected_floppy_interrupt()
{
    current_track = NO_TRACK;
    output_byte(FD_SENSEI);
    printk("%s: unexpected interrupt\n", DEVICE_NAME);
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
    short i;

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
static void reset_floppy(void)
{
    int i;

    do_floppy = reset_interrupt;
    reset = 0;
    current_track = NO_TRACK;
    cur_spec1 = -1;
    cur_rate = -1;
    recalibrate = 1;
    need_configure = 1;
    if (!initial_reset_flag)
	printk("Reset-floppy called\n");
    cli();
    outb_p(current_DOR & ~0x04, FD_DOR);
    for (i = 0; i < 1000; i++)
	__asm__("nop");
    outb(current_DOR, FD_DOR);
    sti();
}

static void floppy_shutdown(void)
{
    cli();
    do_floppy = NULL;
    request_done(0);
    recover = 1;
    reset_floppy();
    sti();
    redo_fd_request();
}

static void shake_done(void)
{
    current_track = NO_TRACK;
    if (inb(FD_DIR) & 0x80)
	request_done(0);
    redo_fd_request();
}

/* FIXME: How does one ANSI'fy the following function? */

static int retry_recal(proc)
     void (*proc) ();
{
    output_byte(FD_SENSEI);
    if (result() == 2 && (ST0 & 0x10) != 0x10)
	return 0;
    do_floppy = proc;
    output_byte(FD_RECALIBRATE);
    output_byte(head << 2 | current_drive);
    return 1;
}

static void shake_zero(void)
{
    if (!retry_recal(shake_zero))
	shake_done();
}

static void shake_one(void)
{
    if (retry_recal(shake_one))
	return;
    do_floppy = shake_done;
    output_byte(FD_SEEK);
    output_byte(head << 2 | current_drive);
    output_byte(1);
}

static void floppy_ready(void)
{
    if (inb(FD_DIR) & 0x80) {
	changed_floppies |= 1 << current_drive;
	buffer_track = -1;
	if (keep_data[current_drive]) {
	    if (keep_data[current_drive] > 0)
		keep_data[current_drive]--;
	} else {
	    if (ftd_msg[current_drive] && current_type[current_drive] != NULL)
		printk("Disk type is undefined after disk change in fd%d\n",
		       current_drive);
	    current_type[current_drive] = NULL;
	    floppy_sizes[current_drive] = MAX_DISK_SIZE;
	}
	/* Forcing the drive to seek makes the "media changed" condition go
	 * away. There should be a cleaner solution for that ...
	 */
	if (!reset && !recalibrate) {
	    if (current_track && current_track != NO_TRACK)
		do_floppy = shake_zero;
	    else
		do_floppy = shake_one;
	    output_byte(FD_RECALIBRATE);
	    output_byte(head << 2 | current_drive);
	    return;
	}
    }
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

static void setup_format_params(void)
{
    unsigned char *here = (unsigned char *) tmp_floppy_area;
    int count, head_shift, track_shift, total_shift;

    /* allow for about 30ms for data transport per track */
    head_shift = floppy->sect / 6;
    /* a ``cylinder'' is two tracks plus a little stepping time */
    track_shift = 2 * head_shift + 1;
    /* count backwards */
    total_shift = floppy->sect -
	((track_shift * track + head_shift * head) % floppy->sect);

    /* XXX: should do a check to see this fits in tmp_floppy_area!! */
    for (count = 0; count < floppy->sect; count++) {
	*here++ = track;
	*here++ = head;
	*here++ = 1 + ((count + total_shift) % floppy->sect);
	*here++ = 2;		/* 512 bytes */
    }
}

static void redo_fd_request(void)
{
    unsigned int block;
    char *buffer_area;
    int device;

    if (CURRENT && CURRENT->dev < 0)
	return;

  repeat:
    if (format_status == FORMAT_WAIT)
	format_status = FORMAT_BUSY;
    if (format_status != FORMAT_BUSY) {
	if (!CURRENT) {
	    if (!fdc_busy)
		printk("FDC access conflict!");
	    fdc_busy = 0;
	    wake_up(&fdc_wait);
	    CLEAR_INTR;
	    return;
	}
	if (MAJOR(CURRENT->dev) != MAJOR_NR)
	    panic("%s: request list destroyed", DEVICE_NAME);
	if (CURRENT->bh) {
	    if (!buffer_locked(CURRENT->bh))
		panic("%s: block not locked", DEVICE_NAME);
	}
    }
    seek = 0;
    probing = 0;
    device = MINOR(CURRENT_DEVICE);
    if (device > 3)
	floppy = (device >> 2) + floppy_type;
    else {			/* Auto-detection */
	floppy = current_type[device & 3];
	if (!floppy) {
	    probing = 1;
	    floppy = base_type[device & 3];
	    if (!floppy) {
		request_done(0);
		goto repeat;
	    }
	    if (CURRENT_ERRORS & 1)
		floppy++;
	}
    }
    if (format_status != FORMAT_BUSY) {
	if (current_drive != CURRENT_DEV) {
	    current_track = NO_TRACK;
	    current_drive = CURRENT_DEV;
	}
	block = CURRENT->sector;
	if (block + 2 > floppy->size) {
	    request_done(0);
	    goto repeat;
	}
	sector = block % floppy->sect;
	block /= floppy->sect;
	head = block % floppy->head;
	track = block / floppy->head;
	seek_track = track << floppy->stretch;
	if (CURRENT->cmd == READ)
	    command = FD_READ;
	else if (CURRENT->cmd == WRITE)
	    command = FD_WRITE;
	else {
	    printk("do_fd_request: unknown command\n");
	    request_done(0);
	    goto repeat;
	}
    } else {
	if (current_drive != (format_req.device & 3))
	    current_track = NO_TRACK;
	current_drive = format_req.device & 3;
	if (((unsigned) format_req.track) >= floppy->track ||
	    (format_req.head & 0xfffe) || probing) {
	    request_done(0);
	    goto repeat;
	}
	head = format_req.head;
	track = format_req.track;
	seek_track = track << floppy->stretch;
	if (seek_track == buffer_track)
	    buffer_track = -1;
	command = FD_FORMAT;
	setup_format_params();
    }
#ifndef TRP_TIMER
    timer_table[FLOPPY_TIMER].expires = jiffies + 10 * HZ;
    timer_active |= 1 << FLOPPY_TIMER;
#endif
    if ((seek_track == buffer_track) && (current_drive == buffer_drive)) {
	buffer_area = floppy_track_buffer +
	    ((sector + head * floppy->sect) << 9);
	if (command == FD_READ) {
	    copy_buffer(buffer_area, CURRENT->buffer);
	    request_done(1);
	    goto repeat;
	} else if (command == FD_WRITE)
	    copy_buffer(CURRENT->buffer, buffer_area);
    }
    if (seek_track != current_track)
	seek = 1;
    sector++;
#ifndef TRP_TIMER
    del_timer(motor_off_timer + current_drive);
#endif
    floppy_on(current_drive);
}

void do_fd_request(void)
{
    cli();
    while (fdc_busy)
	sleep_on(&fdc_wait);
    fdc_busy = 1;
    sti();
    redo_fd_request();
}

static int fd_ioctl(struct inode *inode,
		    struct file *filp, unsigned int cmd, unsigned long param)
{
    struct floppy_struct *this_floppy;
    int i, drive, cnt, okay;

    switch (cmd) {
	RO_IOCTLS(inode->i_rdev, param);
    }
    drive = MINOR(inode->i_rdev);
    switch (cmd) {
    case FDFMTBEG:
	if (!suser())
	    return -EPERM;
	return 0;
    case FDFMTEND:
	if (!suser())
	    return -EPERM;
	cli();
	fake_change |= 1 << (drive & 3);
	sti();
	drive &= 3;
	cmd = FDCLRPRM;
	break;
    case FDGETPRM:
	if (drive > 3)
	    this_floppy = &floppy_type[drive >> 2];
	else if ((this_floppy = current_type[drive & 3]) == NULL)
	    return -ENODEV;
	i =
	    verify_area(VERIFY_WRITE, (void *) param,
			sizeof(struct floppy_struct));
	if (i)
	    return i;
	for (cnt = 0; cnt < sizeof(struct floppy_struct); cnt++)
	    put_fs_byte(((char *) this_floppy)[cnt], (char *) param + cnt);
	return 0;
    case FDFMTTRK:
	if (!suser())
	    return -EPERM;
	if (fd_ref[drive & 3] != 1)
	    return -EBUSY;
	cli();
	while (format_status != FORMAT_NONE)
	    sleep_on(&format_done);
	for (cnt = 0; cnt < sizeof(struct format_descr); cnt++)
	    ((char *) &format_req)[cnt] = get_fs_byte((char *) param + cnt);
	format_req.device = drive;
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
	sti();
	okay = format_status == FORMAT_OKAY;
	format_status = FORMAT_NONE;
	floppy_off(drive & 3);
	wake_up(&format_done);
	return okay ? 0 : -EIO;
    case FDFLUSH:
	if (!permission(inode, 2))
	    return -EPERM;
	cli();
	fake_change |= 1 << (drive & 3);
	sti();
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
	floppy_sizes[drive] = MAX_DISK_SIZE;
	keep_data[drive] = 0;
	break;
    case FDSETPRM:
    case FDDEFPRM:
	memcpy_fromfs(user_params + drive,
		      (void *) param, sizeof(struct floppy_struct));
	current_type[drive] = &user_params[drive];
	floppy_sizes[drive] = user_params[drive].size >> 1;
	if (cmd == FDDEFPRM)
	    keep_data[drive] = -1;
	else {
	    cli();
	    while (fdc_busy)
		sleep_on(&fdc_wait);
	    fdc_busy = 1;
	    sti();
	    outb_p((current_DOR & 0xfc) | drive | (0x10 << drive), FD_DOR);
	    for (cnt = 0; cnt < 1000; cnt++)
		__asm__("nop");
	    if (inb(FD_DIR) & 0x80)
		keep_data[drive] = 1;
	    else
		keep_data[drive] = 0;
	    outb_p(current_DOR, FD_DOR);
	    fdc_busy = 0;
	    wake_up(&fdc_wait);
	}
	break;
    case FDMSGON:
	ftd_msg[drive] = 1;
	break;
    case FDMSGOFF:
	ftd_msg[drive] = 0;
	break;
    case FDSETEMSGTRESH:
	min_report_error_cnt[drive] = (unsigned short) (param & 0x0f);
	break;
    default:
	return -EINVAL;
    }
    return 0;
}

#ifdef TRP_ASM
#define CMOS_READ(addr) ({ \
outb_p(addr,0x70); \
inb_p(0x71); \
})
#else
int CMOS_READ(long addr)
{
    outb_p(addr, 0x70);
    return inb_p(0x71);
}
#endif

static struct floppy_struct *find_base(int drive, int code)
{
    struct floppy_struct *base;

    if (code > 0 && code < 5) {
	base = &floppy_types[(code - 1) * 2];
	printk("fd%d is %s", drive, base->name);
	return base;
    }
    printk("fd%d is unknown type %d", drive, code);
    return NULL;
}

static void config_types(void)
{
    printk("Floppy drive(s): ");
    base_type[0] = find_base(0, (CMOS_READ(0x10) >> 4) & 15);
    if (((CMOS_READ(0x14) >> 6) & 1) == 0)
	base_type[1] = NULL;
    else {
	printk(", ");
	base_type[1] = find_base(1, CMOS_READ(0x10) & 15);
    }
    base_type[2] = base_type[3] = NULL;
    printk("\n");
}

/*
 * floppy_open check for aliasing (/dev/fd0 can be the same as
 * /dev/PS0 etc), and disallows simultaneous access to the same
 * drive with different device numbers.
 */
static int floppy_open(struct inode *inode, struct file *filp)
{
    int drive, old_dev;

    drive = inode->i_rdev & 3;
    old_dev = fd_device[drive];
    if (fd_ref[drive])
	if (old_dev != inode->i_rdev)
	    return -EBUSY;
    fd_ref[drive]++;
    fd_device[drive] = inode->i_rdev;
    buffer_drive = buffer_track = -1;
    if (old_dev && old_dev != inode->i_rdev)
	invalidate_buffers(old_dev);
    if (filp && filp->f_mode)
	check_disk_change(inode->i_rdev);
    return 0;
}

static void floppy_release(struct inode *inode, struct file *filp)
{
    sync_dev(inode->i_rdev);
    if (!fd_ref[inode->i_rdev & 3]--) {
	printk("floppy_release with fd_ref == 0");
	fd_ref[inode->i_rdev & 3] = 0;
    }
}

static struct file_operations floppy_fops = {
    NULL,			/* lseek - default */
    block_read,			/* read - general block-dev read */
    block_write,		/* write - general block-dev write */
    NULL,			/* readdir - bad */
    NULL,			/* select */
    fd_ioctl,			/* ioctl */
    NULL,			/* mmap */
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
    printk("%s: weird interrupt ignored (%d)\n", DEVICE_NAME, result());
    reset = 1;
    CLEAR_INTR;			/* ignore only once */
}

static void floppy_interrupt(int unused)
{
    void (*handler) () = DEVICE_INTR;

    DEVICE_INTR = NULL;
    if (!handler)
	handler = unexpected_floppy_interrupt;
    handler();
}

/*
 * This is the floppy IRQ description. The SA_INTERRUPT in sa_flags
 * means we run the IRQ-handler with interrupts disabled.
 */
static struct sigaction floppy_sigaction = {
    floppy_interrupt,
    0,
    SA_INTERRUPT,
    NULL
};

void floppy_init(void)
{
    extern int blk_size[];

    outb(current_DOR, FD_DOR);
    if (register_blkdev(MAJOR_NR, "fd", &floppy_fops)) {
	printk("Unable to get major %d for floppy\n", MAJOR_NR);
	return;
    }
    blk_size[MAJOR_NR] = floppy_sizes;
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
#ifndef TRP_TIMER
    timer_table[FLOPPY_TIMER].fn = floppy_shutdown;
    timer_active &= ~(1 << FLOPPY_TIMER);
#endif
    config_types();
#ifdef TRP_SIG
    if (irqaction(FLOPPY_IRQ, &floppy_sigaction))
	printk("Unable to grab IRQ%d for the floppy driver\n", FLOPPY_IRQ);
#endif
    if (request_dma(FLOPPY_DMA))
	printk("Unable to grab DMA%d for the floppy driver\n", FLOPPY_DMA);
    /* Try to determine the floppy controller type */
    DEVICE_INTR = ignore_interrupt;	/* don't ask ... */
    output_byte(FD_VERSION);	/* get FDC version code */
    if (result() != 1) {
	printk("%s: FDC failed to return version byte\n", DEVICE_NAME);
	fdc_version = FDC_TYPE_STD;
    } else
	fdc_version = reply_buffer[0];
    if (fdc_version != FDC_TYPE_STD)
	printk("%s: FDC version 0x%x\n", DEVICE_NAME, fdc_version);
#ifndef FDC_FIFO_UNTESTED
    fdc_version = FDC_TYPE_STD;	/* force std fdc type; can't test other. */
#endif

    /* Not all FDCs seem to be able to handle the version command
     * properly, so force a reset for the standard FDC clones,
     * to avoid interrupt garbage.
     */

    if (fdc_version == FDC_TYPE_STD) {
	initial_reset_flag = 1;
	reset_floppy();
    }
}

#endif

#endif
