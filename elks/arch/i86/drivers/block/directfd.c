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
 * - Update DMA code
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

/* This is confusing. DEVICE_INTR is the do_floppy variable.
 * The code is sometimes using the macro, some times the variable.
 * It may seem this is a trick to get GCC to shut up ...
 */
#ifdef DEVICE_INTR	/* from blk.h */
void (*DEVICE_INTR) () = NULL;

#define SET_INTR(x) (DEVICE_INTR = (x))
#define CLEAR_INTR SET_INTR(NULL)
#else
#define CLEAR_INTR
#endif

#define _MK_LINADDR(seg, offs) ((unsigned long)((((unsigned long)(seg)) << 4) + (unsigned)(offs)))

//#define debug_blkdrv    printk
#define debug_blkdrv(...)
//#define dfd_debug
#ifdef dfd_debug
#define DEBUG printk
#else
#define DEBUG(...)
#endif

/* Formatting code is currently untested, don't waste the space */
//#define INCLUDE_FD_FORMATTING

static int initial_reset_flag = 0;
static int need_configure = 1;	/* for 82077 */
static int recalibrate = 0;
static int reset = 0;
static int recover = 0;		/* recalibrate immediately after resetting */
static int seek = 0;

#ifdef CONFIG_BLK_DEV_CHAR
static int nr_sectors;		/* only when raw access */
#endif

/* BIOS floppy motor timeout counter - FIXME leave this while BIOS driver present */
static unsigned char __far *fl_timeout = (void __far *)0x440L;

static unsigned char current_DOR = 0x0C;
static unsigned char running = 0; /* keep track of motors already running */
/* NOTE: current_DOR tells which motor(s) have been commanded to run,
 * 'running' tells which ones are actually running. The difference is subtle - 
 * the spinup time, typical .5 secs */

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
 * The 8237 DMA controller cannot access data above 1MB on the origianl PC 
 * (4 bit page register). The AT is different, the page reg is 8 bits while the
 * 2nd DMA controller transfers words only, not bytes and thus up to 128k bytes
 * in one 'swoop'.
 */
#define LAST_DMA_ADDR	(0x100000L - BLOCK_SIZE) /* stick to the 1M limit */

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
    /* totSectors/secPtrack/heads/tracks/stretch/gap/Drate/S&Hrates/fmtGap/nm/  */
};

/*
 * Auto-detection. Each drive type has a pair of formats which are
 * used in succession to try to read the disk. If the FDC cannot lock onto
 * the disk, the next format is tried. This uses the variable 'probing'.
 * NOTE: Ignore the duplicates, they're there for a reason.
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
//struct floppy_struct user_params[4];

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
static int ftd_msg[4] = { 0, 0, 0, 0 };

static int fd_ref[4] = { 0, 0, 0, 0 };	/* device reference counter */

/* Synchronization of FDC access. */
static int format_status = FORMAT_NONE, fdc_busy = 0;
static struct wait_queue fdc_wait;
//static struct wait_queue format_done;

/* Errors during formatting are counted here. */
//static int format_errors;

/* Format request descriptor. */
//static struct format_descr format_req;

/* Current device number. */
#define CURRENT_DEVICE (CURRENT->rq_dev)

/* Current error count. */
#define CURRENT_ERRORS (CURRENT->rq_errors)

/*
 * Threshold for reporting FDC errors to the console.
 * Setting this to zero may flood your screen when using
 * ultra cheap floppies ;-)
 */
static unsigned short min_report_error_cnt[4] = { 2, 2, 2, 2 };

/*
 * Rate is 0 for 500kb/s, 1 for 300kbps, 2 for 250kbps
 * Spec1 is 0xSH, where S is stepping rate (F=1ms, E=2ms, D=3ms etc),
 * H is head unload time, time the FDC will wait before unloading
 * the head after a command that accesses the disk (1=16ms, 2=32ms, etc)
 *
 * Spec2 is (HLD<<1 | ND), where HLD is head load time (1=2ms, 2=4 ms etc)
 * and ND is set means no DMA. Hardcoded to 6 (HLD=6ms, use DMA).
 */

/*
 * The block buffer is used for all writes, for formatting and for reads
 * in case track buffering doesn't work or has been turned off.
 */
#define WORD_ALIGNED    __attribute__((aligned(2)))
static char tmp_floppy_area[BLOCK_SIZE] WORD_ALIGNED; /* for now FIXME to be removed */

#ifdef CHECK_DISK_CHANGE
int check_disk_change(kdev_t);
#endif

static void redo_fd_request(void);
static void recal_interrupt(void);
static void floppy_shutdown(void);
static void motor_off_callback(int);

/*
 * These are global variables, as that's the easiest way to give
 * information to interrupts. They are the data used for the current
 * request.
 */
#define NO_TRACK 255

static int read_track = 0;	/* set to read entire track */
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

static void floppy_ready(void);

static void delay_loop(int cnt)
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
 * NOTE: The argument (nr) is silently ignored, current_drive being used instead.
 * is this OK?
  */
static void floppy_select(unsigned int nr)
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

    /* It is not obvious why select should take any time at all ... HS */
    /* The select_callback calls floppy_ready() */
    /* The callback is NEVER called unless several drives are active concurrently */
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
static void floppy_on(int nr)
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
	motor_on_timer[nr].tl_expires = jiffies + HZ/2;	/* TEAC 1.44M says 'waiting time' 505ms,
							 * may be too little for 5.25in drives. */
	add_timer(&motor_on_timer[nr]);

	current_DOR &= 0xFC;	/* remove drive select */
	current_DOR |= mask;	/* set motor select */
	current_DOR |= nr;	/* set drive select */
	outb(current_DOR, FD_DOR);
    }	// EXPERIMENTAL - moved this one ...

    DEBUG("flpON0x%x;", current_DOR);
}

/* floppy_off (and floppy_on) are called from upper layer routines when a
 * block request is started or ended respectively. It's extremely important
 * that the motor off timer is slow enough not to affect performance. 3 
 * seconds is a fair compromise.
 */
static void floppy_off(unsigned int nr)
{
    del_timer(&motor_off_timer[nr]);
    motor_off_timer[nr].tl_expires = jiffies + 3 * HZ;
    add_timer(&motor_off_timer[nr]);
    DEBUG("flpOFF-\n");
}

void request_done(int uptodate)
{
    /* FIXME: Is this the right place to delete this timer? */
    del_timer(&fd_timeout);

#ifdef INCLUDE_FD_FORMATTING
    if (format_status != FORMAT_BUSY)
	end_request(uptodate);
    else {
	format_status = uptodate ? FORMAT_OKAY : FORMAT_ERROR;
	wake_up(&format_done);
    }
#else
    end_request(uptodate);
#endif
}

#ifdef CHECK_DISK_CHANGE
/*
 * The check_media_change entry in struct file_operations (fs.h) is not
 * part of the 'normal' setup (only BLOAT_FS), so we're ignoring it for now,
 * assuming the user is smart enough to umount before media changes - or
 * ready for the consequences.
 */ 

/*
 * floppy-change is never called from an interrupt, so we can relax a bit
 * here, sleep etc. Note that floppy-on tries to set current_DOR to point
 * to the desired drive, but it will probably not survive the sleep if
 * several floppies are used at the same time: thus the loop.
 */
static unsigned int changed_floppies = 0, fake_change = 0;

int floppy_change(struct buffer_head *bh)
{
    unsigned int mask = 1 << ((bh->b_dev & 0x03) >> MINOR_SHIFT;

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

static void setup_DMA(void)
{
    unsigned long dma_addr;
    unsigned int count, physaddr;
    int use_xms;
    struct request *req = CURRENT;

#pragma GCC diagnostic ignored "-Wshift-count-overflow"
    use_xms = req->rq_seg >> 16; /* will ne nonzero only if XMS configured & XMS buffer */
    physaddr = (req->rq_seg << 4) + (unsigned int)req->rq_buffer;

#ifdef CONFIG_BLK_DEV_CHAR
    count = nr_sectors ? nr_sectors<<9 : BLOCK_SIZE;
#else
    count = BLOCK_SIZE;
#endif
    if (use_xms || (physaddr + (unsigned int)count) < physaddr)
	dma_addr = LAST_DMA_ADDR + 1;	/* force use of bounce buffer */
    else
	dma_addr = _MK_LINADDR(req->rq_seg, req->rq_buffer);

    DEBUG("setupDMA ");

#ifdef INCLUDE_FD_FORMATTING
    if (command == FD_FORMAT) {
	dma_addr = _MK_LINADDR(kernel_ds, tmp_floppy_area);
	count = floppy->sect * 4;
    }
#endif
    if (read_track) {	/* mark buffer-track bad, in case all this fails.. */
	buffer_drive = buffer_track = -1;
	count = floppy->sect << 9;	/* sects/trk (one side) times 512 */
	if (floppy->sect & 1 && !head)
	    count += 512; /* add one if head=0 && sector count is odd */
	dma_addr = _MK_LINADDR(DMASEG, 0);
    } else if (dma_addr >= LAST_DMA_ADDR) {
	dma_addr = _MK_LINADDR(kernel_ds, tmp_floppy_area); /* use bounce buffer */
	if (command == FD_WRITE) {
	    xms_fmemcpyw(tmp_floppy_area, kernel_ds, CURRENT->rq_buffer, CURRENT->rq_seg, BLOCK_SIZE/2);
	}
    }
    DEBUG("%d/%lx;", count, dma_addr);
    clr_irq();
    disable_dma(FLOPPY_DMA);
    clear_dma_ff(FLOPPY_DMA);
    set_dma_mode(FLOPPY_DMA,
		 (command == FD_READ) ? DMA_MODE_READ : DMA_MODE_WRITE);
    set_dma_addr(FLOPPY_DMA, dma_addr);
    set_dma_count(FLOPPY_DMA, count);
    enable_dma(FLOPPY_DMA);
    set_irq();
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
    printk("Getstatus timed out\n");
    return -1;
}

static void bad_flp_intr(void)
{
    int errors;

    DEBUG("bad_flpI-");
    current_track = NO_TRACK;
#ifdef INCLUDE_FD_FORMATTING
    if (format_status == FORMAT_BUSY)
	errors = ++format_errors;
    else 
#endif
    if (!CURRENT) {
	printk("%s: no current request\n", DEVICE_NAME);
	reset = recalibrate = 1;
	return;
    } else
	errors = ++CURRENT->rq_errors;
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
#ifdef FDC_FIFO_UNTESTED
    if (need_configure && (fdc_version == FDC_TYPE_82077)) {
	/* Enhanced version with FIFO & vertical recording. */
	output_byte(FD_CONFIGURE);
	output_byte(0);
	output_byte(0x1A);	/* FIFO on, polling off, 10 byte threshold */
	output_byte(0);		/* precompensation from track 0 upwards */
	need_configure = 0;
	printk("%s: FIFO enabled\n", DEVICE_NAME);
    }
#endif
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
	if (ST1 & ST1_WP) {
	    printk("%s: Drive %d is write protected", DEVICE_NAME,
		   current_drive);
	    request_done(0);
	    bad = 0;
	} else if (ST1 & ST1_OR) {
	    if (ftd_msg[ST0 & ST0_DS])
		printk("%s: Over/Under-run - retrying", DEVICE_NAME);
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
		printk("unknown error. ST[0-3]: 0x%x 0x%x 0x%x 0x%x",
		       ST0, ST1, ST2, ST3);
	    }
	    CURRENT->rq_errors++; /* may want to increase this even more, doesn't make 
	    		 * sense to re-try most of these conditions more 
			 * than the reporting threshold. */
			/* FIXME: Need smarter retry/error reporting scheme */
	}
	printk("\n");
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
	int drive = (MINOR(CURRENT->rq_dev) >> MINOR_SHIFT) & 3;

	if (ftd_msg[drive])
	    printk("Auto-detected floppy type %s in df%d\n",
		   floppy->name, drive);
	current_type[drive] = floppy;
	probing = 0;
    }
    if (read_track) {
	buffer_track = (seek_track << 1) + head; /* This encoding is ugly, should
						  * use block-start, block-end instead */
	buffer_drive = current_drive;
	buffer_area = (unsigned char *)(sector << 9);
	DEBUG("rd:%04x:%08lx->%04x:%04x;", DMASEG, buffer_area,
		(unsigned long)CURRENT->rq_seg, CURRENT->rq_buffer);
	xms_fmemcpyw(CURRENT->rq_buffer, CURRENT->rq_seg, buffer_area, DMASEG, BLOCK_SIZE/2);
    } else if (command == FD_READ 
#ifndef CONFIG_FS_XMS_BUFFER
	   && _MK_LINADDR(CURRENT->rq_seg, CURRENT->rq_buffer) >= LAST_DMA_ADDR
#endif
	) {
	/* if the dest buffer is out of reach for DMA (always the case if using
	 * XMS buffers) we need to read/write via the bounce buffer */
	xms_fmemcpyw(CURRENT->rq_buffer, CURRENT->rq_seg, tmp_floppy_area, kernel_ds, BLOCK_SIZE/2);
	printk("directfd: illegal buffer usage, rq_buffer %04x:%04x\n", 
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
void setup_rw_floppy(void)
{
    DEBUG("setup_rw-");
    setup_DMA();
    do_floppy = rw_interrupt;
    output_byte(command);
    output_byte(head << 2 | current_drive);

    if (command != FD_FORMAT) {
	output_byte(track);
	output_byte(head);
	if (read_track)
	    output_byte(1);	/* always start at 1 */
	else
	    output_byte(sector+1); 

	output_byte(2);		/* sector size = 512 */
	output_byte(floppy->sect);
	output_byte(floppy->gap);
	output_byte(0xFF);	/* sector size, 0xff unless sector size==0 (128b) */
    } else {
	output_byte(2);		/* sector size = 512 */
	output_byte(floppy->sect * 2); /* sectors per cyl */
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
    DEBUG("seekI-");
    output_byte(FD_SENSEI);
    if (result() != 2 || (ST0 & 0xF8) != 0x20 || ST1 != seek_track) {
	printk("%s%d: seek failed\n", DEVICE_NAME, current_drive);
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
static void transfer(void)
{
#ifdef CONFIG_TRACK_CACHE
    read_track = (command == FD_READ) && (CURRENT_ERRORS < 4) &&
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
    if (reset)	/* If something happened in output_byte() */
	redo_fd_request();
}

static void recalibrate_floppy(void)
{
    DEBUG("recal-");
    recalibrate = 0;
    current_track = 0;
    do_floppy = recal_interrupt;
    output_byte(FD_RECALIBRATE);
    output_byte(current_drive);

    /* this may not make sense: We're waiting for recal_interrupt
     * why redo_fd_request here when recal_interrupt is doing it ??? */
    /* 'reset' gets set in recal_interrupt, maybe that's it ??? */
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
    if (ST0 & 0x10)		/* Look for UnitCheck, which will happen regularly
    				 * on 80 track drives because RECAL only steps 77 times */
	recalibrate_floppy();	/* FIXME: may loop, should limit nr of recalibrates */
    else
	redo_fd_request();
}

static void unexpected_floppy_interrupt(void)
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

    DEBUG("rstI-");
    for (i = 0; i < 4; i++) {
	output_byte(FD_SENSEI);
	(void) result();
    }
    //DEBUG("1-");
    output_byte(FD_SPECIFY);
    output_byte(cur_spec1);	/* hut etc */
    output_byte(6);		/* Head load time =6ms, DMA */
    //DEBUG("2-");
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

    DEBUG("[%u]rst-", (unsigned int)jiffies);
    do_floppy = reset_interrupt;
    reset = 0;
    current_track = NO_TRACK;
    cur_spec1 = -1;
    cur_rate = -1;
    recalibrate = 1;
    need_configure = 1;
    if (!initial_reset_flag)
	printk("Reset-floppy called\n");
    clr_irq();
    outb_p(current_DOR & ~0x04, FD_DOR);
    delay_loop(1000);
    outb(current_DOR, FD_DOR);
    set_irq();
}

static void floppy_shutdown(void)
{
    printk("[%u]shtdwn0x%x|%x-", (unsigned int)jiffies, current_DOR, running);
    do_floppy = NULL;
    request_done(0);
    recover = 1;
    reset_floppy();
    redo_fd_request();
}

static void shake_done(void)
{
    /* Need SENSEI to clear the interrupt per spec, required by QEMU, not by
     * real hardware	*/
    output_byte(FD_SENSEI);	/* TESTING FIXME */
    result();
    DEBUG("shD0x%x-", ST0);
    current_track = NO_TRACK;
    if (inb(FD_DIR) & 0x80 || ST0 & 0xC0)
	request_done(0);	/* still errors: just fail */
    else
	redo_fd_request();
}

/*
 * The result byte after the SENSEI cmd is ST3, not ST0
 */
static int retry_recal(void (*proc)())
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
	return;	/* just return if retry_recal initiated a RECAL */
    do_floppy = shake_done;
    output_byte(FD_SEEK);
    output_byte(head << 2 | current_drive);
    output_byte(1);
}

static void floppy_ready(void)
{
    DEBUG("RDY0x%x,%d,%d-", inb(FD_DIR), reset, recalibrate);
    if (inb(FD_DIR) & 0x80) {	/* set if disk changed since last cmd (AT ++) */
#ifdef CHECK_DISK_CHANGE
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
	    if (ftd_msg[current_drive] && current_type[current_drive] != NULL)
		printk("Disk type is undefined after disk change in df%d\n",
		       current_drive);
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

#ifdef INCLUDE_FD_FORMATTING
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
#endif

static void redo_fd_request(void)
{
    unsigned int start;
    struct request *req;
    int device, drive;

    /* cannot use the INIT_REQUEST macro since req=NULL is OK */

    if (CURRENT && ((int)CURRENT->rq_dev == -1U))
	return;
  repeat:
    req = CURRENT;
    if (format_status == FORMAT_WAIT)
	format_status = FORMAT_BUSY;
    if (format_status != FORMAT_BUSY) {
	if (!req) {
	    if (!fdc_busy)
		printk("FDC access conflict!");
	    fdc_busy = 0;
	    wake_up(&fdc_wait);
	    CLEAR_INTR;
	    return;
	}
	if (MAJOR(req->rq_dev) != MAJOR_NR)
	    panic("%s: request list destroyed", DEVICE_NAME);
	if (req->rq_bh && !EBH(req->rq_bh)->b_locked)
		panic("%s: block not locked", DEVICE_NAME);
    }
    seek = 0;
    probing = 0;
    device = MINOR(CURRENT_DEVICE) >> MINOR_SHIFT;
    drive = device & 3;
    if (device > 3)
	floppy = (device >> 2) + floppy_type;
    else {			/* Auto-detection */
	floppy = current_type[drive];
	if (!floppy) {
	    probing = 1;
	    floppy = base_type[drive];
	    if (!floppy) {
		request_done(0);
		goto repeat;
	    }
	    if (CURRENT_ERRORS & 1)
		floppy++;
	}
    }
    DEBUG("[%u]redo-%c %d(%s) bl %u;", (unsigned int)jiffies, 
		req->rq_cmd == WRITE? 'W':'R', device, floppy->name, req->rq_sector);
    debug_blkdrv("df%d: %c sector %ld\n", CURRENT_DEVICE & 3, req->rq_cmd==WRITE? 'W' : 'R', req->rq_sector);
    if (format_status != FORMAT_BUSY) {
    	unsigned int tmp;
	if (current_drive != drive) {
	    current_track = NO_TRACK;
	    current_drive = drive;
	}
	start = (unsigned int) req->rq_sector;
	if (start + 2 > floppy->size) {
	    request_done(0);
	    goto repeat;	/* FIXME: Should probably exit here */
				/* or at least increment an error counter */
	}
#ifdef CONFIG_BLK_DEV_CHAR
	nr_sectors = req->rq_nr_sectors;	/* non-zero if raw io */
#endif
	sector = start % floppy->sect;
	tmp = start / floppy->sect;
	head = tmp % floppy->head;
	track = tmp / floppy->head;
	seek_track = track << floppy->stretch;
	DEBUG("%d:%d:%d:%d; ", start, sector, head, track);
	if (req->rq_cmd == READ)
	    command = FD_READ;
	else if (req->rq_cmd == WRITE)
	    command = FD_WRITE;
	else {
	    printk("redo_fd_request: unknown command\n");
	    request_done(0);
	    goto repeat;
	}
    }
#ifdef INCLUDE_FD_FORMATTING
    else {	/* Format drive */
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
	if ((seek_track << 1) + head == buffer_track)
	    buffer_track = -1;
	command = FD_FORMAT;
	setup_format_params();
    }
#endif

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
    if (CURRENT) CURRENT->rq_errors = 0;	// EXPERIMENTAL
    while (fdc_busy)
	sleep_on(&fdc_wait);
    fdc_busy = 1;
    redo_fd_request();
}

static int fd_ioctl(struct inode *inode,
		    struct file *filp, unsigned int cmd, unsigned int param)
{
/* FIXME: Get this back in when everything else is working */
/* Needs a big cleanup */
    struct floppy_struct *this_floppy;
    int drive, err = -EINVAL;
    struct hd_geometry *loc = (struct hd_geometry *) param;

    if (!inode || !inode->i_rdev)
	return -EINVAL;
    drive = MINOR(inode->i_rdev) >> MINOR_SHIFT;
    if (drive > 3)
	this_floppy = &floppy_type[drive >> 2];
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
	//return verified_memcpy_tofs((char *)param,
	//	    (char *)this_floppy, sizeof(struct floppy_struct));
	return err;
#ifdef INCLUDE_FD_FORMATTING
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
    case FDMSGON:
	ftd_msg[drive] = 1;
	break;
    case FDMSGOFF:
	ftd_msg[drive] = 0;
	break;
    case FDSETEMSGTRESH:
	min_report_error_cnt[drive] = (unsigned short) (param & 0x0f);
	break;
#endif
    default:
	return -EINVAL;
    }
    return err;
}

static int CMOS_READ(int addr)
{
    outb_p(addr, 0x70);
    return inb_p(0x71);
}

static struct floppy_struct *find_base(int drive, int code)
{
    struct floppy_struct *base;

    if (code > 0 && code < 5) {
	base = &floppy_types[(code - 1) * 2];
	printk("df%d is %s (%d)", drive, base->name, code);
	return base;
    }
    printk("df%d is unknown type %d", drive, code);
    return NULL;
}

static void config_types(void)
{
    printk("Floppy drive(s) [CMOS]: ");
    base_type[0] = find_base(0, (CMOS_READ(0x10) >> 4) & 0xF);
    if (((CMOS_READ(0x14) >> 6) & 1) == 0)
	base_type[1] = NULL;
    else {
	printk(", ");
	base_type[1] = find_base(1, CMOS_READ(0x10) & 0xF);
    }
    base_type[2] = base_type[3] = NULL;
    printk("\n");
}

static int floppy_open(struct inode *inode, struct file *filp)
{
    int drive, dev;

    dev = drive = DEVICE_NR(inode->i_rdev);
    fd_ref[dev]++;
    buffer_drive = buffer_track = -1;	/* FIXME: Don't invalidate buffer if
					 * this is a reopen of the currently
					 * bufferd drive. */

#ifdef CHECK_DISK_CHANGE
    if (filp && filp->f_mode)
	check_disk_change(inode->i_rdev);
#endif

    if (drive > 3)		/* forced floppy type */
	floppy = (drive >> 2) + floppy_type;
    else {			/* Auto-detection */
	floppy = current_type[dev];
	if (!floppy) {
	    probing = 1;
	    floppy = base_type[dev];
	    if (!floppy)
		return -ENXIO;
	}
    }
    inode->i_size = ((sector_t)(floppy->size)) << 9;	/* NOTE: assumes sector size 512 */
    debug_blkdrv("df%d: open dv %x, sz %lu, %s\n", drive,
		inode->i_rdev, inode->i_size, floppy->name);

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
    printk("%s: weird interrupt ignored (%d)\n", DEVICE_NAME, result());
    reset = 1;
    CLEAR_INTR;			/* ignore only once */
}

static void floppy_interrupt(int unused, struct pt_regs *unused1)
{
    void (*handler) () = DEVICE_INTR;

    DEVICE_INTR = NULL;
    if (!handler)
	handler = unexpected_floppy_interrupt;
    //printk("$");
    handler();
}

void INITPROC floppy_init(void)
{
    int err;

    outb(current_DOR, FD_DOR);	/* all motors off, DMA, /RST  (0x0c) */
    if (register_blkdev(MAJOR_NR, DEVICE_NAME, &floppy_fops)) {
	printk("Unable to get major %d for floppy\n", MAJOR_NR);
	return;
    }
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;

    config_types();
    err = request_irq(FLOPPY_IRQ, floppy_interrupt, INT_GENERIC);
    if (err) {
	printk("Unable to grab IRQ%d for the floppy driver\n", FLOPPY_IRQ);
	return;	/* should be able to signal failure back to the caller */
    }

    if (request_dma(FLOPPY_DMA, (void *)DEVICE_NAME))
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
	printk("%s: Direct floppy driver, FDC (%s) @ irq %d, DMA %d\n", DEVICE_NAME, 
		fdc_version == 0x80 ? "8272A" : "82077", FLOPPY_IRQ, FLOPPY_DMA);
#ifndef FDC_FIFO_UNTESTED
    fdc_version = FDC_TYPE_STD;	/* force std fdc type; can't test other. */
#endif

    /* Not all FDCs seem to be able to handle the version command
     * properly, so force a reset for the standard FDC clones,
     * to avoid interrupt garbage.
     */
#if 1	/* testing FIXME --  the FDC has just been reset by the BIOS, do we need this? */
    if (fdc_version == FDC_TYPE_STD) {
	initial_reset_flag = 1;
	reset_floppy();
    }
#endif
}

#if 0
/* replace separate DMA handler later - this is much more compact and efficient */

/*===========================================================================*
 *				dma_setup (from minix driver)		     *
 *===========================================================================*/
static void dma_setup(int opcode)
{
/* The IBM PC can perform DMA operations by using the DMA chip.  To use it,
 * the DMA (Direct Memory Access) chip is loaded with the 20-bit memory address
 * to be read from or written to, the byte count minus 1, and a read or write
 * opcode.  This routine sets up the DMA chip.  Note that the chip is not
 * capable of doing a DMA across a 64K boundary (e.g., you can't read a
 * 512-byte block starting at physical address 65520).
 */

  /* Set up the DMA registers.  (The comment on the reset is a bit strong,
   * it probably only resets the floppy channel.)
   */
  outb(DMA_INIT, DMA_RESET_VAL);	/* reset the dma controller */
  outb(DMA_FLIPFLOP, 0);		/* write anything to reset it */
  outb(DMA_MODE, opcode == DEV_SCATTER ? DMA_WRITE : DMA_READ);
  outb(DMA_ADDR, (unsigned) tmp_phys >>  0);
  outb(DMA_ADDR, (unsigned) tmp_phys >>  8);
  outb(DMA_TOP, (unsigned) (tmp_phys >> 16));
  outb(DMA_COUNT, (SECTOR_SIZE - 1) >> 0);
  outb(DMA_COUNT, (SECTOR_SIZE - 1) >> 8);
  outb(DMA_INIT, 2);			/* some sort of enable */
}
#endif
