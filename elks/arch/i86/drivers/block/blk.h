#ifndef _BLK_H
#define _BLK_H

#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/kdev_t.h>
/*#include <linuxmt/locks.h>*/
#include <linuxmt/genhd.h>
#include <linuxmt/config.h>

/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and 'waiting' is used to wait for
 * read/write completion.
 */

struct request {
    kdev_t rq_dev;		/* -1 if no request */
    __u8 rq_cmd;		/* READ or WRITE */
    __u8 rq_status;
    sector_t rq_sector;
    char *rq_buffer;
    seg_t rq_seg;		/* Used by swapper */
    struct buffer_head *rq_bh;
    struct request *rq_next;

#ifdef BLOAT_FS

/* This may get used for dealing with waiting for requests later
 * but for now it is just not used
 */
    struct task_struct *rq_waiting;
    sector_t rq_nr_sectors;	/* always 2 */
    sector_t rq_current_nr_sectors;
    __u8 rq_errors;

#endif

};

#define RQ_INACTIVE	0
#define RQ_ACTIVE	1

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 *
 * Update: trying with writes being preferred due to test
 * by Alessandro Rubini..
 */

#define IN_ORDER(s1,s2) \
((s1)->rq_cmd > (s2)->rq_cmd || ((s1)->rq_cmd == (s2)->rq_cmd && \
((s1)->rq_dev < (s2)->rq_dev || (((s1)->rq_dev == (s2)->rq_dev && \
(s1)->rq_sector < (s2)->rq_sector)))))

struct blk_dev_struct {
    void (*request_fn) ();
    struct request *current_request;
};

#define SECTOR_MASK 2		/* 1024 logical 512 physical */

#define SUBSECTOR(block) (CURRENT->rq_current_nr_sectors > 0)

extern struct blk_dev_struct blk_dev[MAX_BLKDEV];

#ifdef MULTI_BH

extern struct wait_queue *wait_for_request;

#endif

extern void resetup_one_dev();

extern void rd_load();
extern void rd_init();

extern void xd_init();

#ifdef MAJOR_NR

/*
 * Add entries as needed. Currently the only block devices
 * supported are hard-disks and floppies.
 */

#ifdef RAMDISK
/* ram disk */
#define DEVICE_NAME "ramdisk"
#define DEVICE_REQUEST do_rd_request
#define DEVICE_NR(device) ((device) & 7)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#endif

#ifdef SSDDISK
#define DEVICE_NAME "ssddisk"
#define DEVICE_REQUEST do_ssd_request
#define DEVICE_NR(device) ((device) & 3)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif
#ifdef FLOPPYDISK

static void floppy_on();	/*(unsigned int nr); */
static void floppy_off();	/*(unsigned int nr); */

#define DEVICE_NAME "floppy"
#define DEVICE_INTR do_floppy
#define DEVICE_REQUEST do_fd_request
#define DEVICE_NR(device) ((device) & 3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))

#endif
#ifdef ATDISK

/* harddisk: timeout is 6 seconds.. */
#define DEVICE_NAME "harddisk"

#if 0

#define DEVICE_INTR do_hd
#define DEVICE_TIMEOUT HD_TIMER
#define TIMEOUT_VALUE 600

#endif

#define DEVICE_REQUEST do_directhd_request
#define DEVICE_NR(device) (MINOR(device)>>6)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif
#ifdef XTDISK

#define DEVICE_NAME "xt disk"
#define DEVICE_REQUEST do_xd_request
#define DEVICE_NR(device) (MINOR(device) >> 6)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef BIOSDISK
#define DEVICE_NAME "BIOSHD"
#define DEVICE_REQUEST do_bioshd_request
#define DEVICE_NR(device) (MINOR(device)>>6)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#endif

#ifdef METADISK
#define DEVICE_NAME "meta"
#define DEVICE_REQUEST do_meta_request
#define DEVICE_NR(device) (MINOR(device))
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#endif

#ifndef CURRENT
#define CURRENT (blk_dev[MAJOR_NR].current_request)
#endif

#define CURRENT_DEV DEVICE_NR(CURRENT->rq_dev)

#ifdef DEVICE_INTR

void (*DEVICE_INTR) () = NULL;

#endif

#ifdef DEVICE_TIMEOUT

#define SET_TIMER \
((timer_table[DEVICE_TIMEOUT].expires = jiffies + TIMEOUT_VALUE), \
(timer_active |= 1<<DEVICE_TIMEOUT))

#define CLEAR_TIMER \
timer_active &= ~(1<<DEVICE_TIMEOUT)

#define SET_INTR(x) \
if ((DEVICE_INTR = (x)) != NULL) \
	SET_TIMER; \
else \
	CLEAR_TIMER;

#else

#define SET_INTR(x) (DEVICE_INTR = (x))

#endif

static void (DEVICE_REQUEST) ();

static void end_request(int uptodate)
{
    register struct request *req;
    register struct buffer_head *bh;
    struct task_struct *p;

    req = CURRENT;

#ifdef BLOAT_FS

    req->rq_errors = 0;

#endif

    if (!uptodate) {
	printk("%s:I/O error\n", DEVICE_NAME);
	printk("dev %x, sector %lu\n", req->rq_dev, req->rq_sector);

#ifdef MULTI_BH

	req->rq_nr_sectors--;
	req->rq_nr_sectors &= ~SECTOR_MASK;
	req->rq_sector += (BLOCK_SIZE / 512);
	req->rq_sector &= ~SECTOR_MASK;

#endif

    }

    bh = req->rq_bh;

#ifdef BLOAT_FS

    req->rq_bh = bh->b_reqnext;
    bh->b_reqnext = NULL;

#endif

    i_cli();
    bh->b_uptodate = uptodate;
    i_sti();
    unlock_buffer(bh);

#ifdef BLOAT_FS

    if ((bh = req->rq_bh) != NULL) {
	req->rq_current_nr_sectors = bh->b_size >> 9;
#if 0
	req->rq_current_nr_sectors = 1024 >> 9;
#endif
	if (req->rq_nr_sectors < req->rq_current_nr_sectors) {
	    req->rq_nr_sectors = req->rq_current_nr_sectors;
	    printk("end_request: buffer-list destroyed\n");
	}
	req->rq_buffer = bh->b_data;
	return;
    }
#endif
    DEVICE_OFF(req->dev);
    CURRENT = req->rq_next;
#ifdef BLOAT_FS
    if ((p = req->rq_waiting) != NULL) {
	req->rq_waiting = NULL;
	p->state = TASK_RUNNING;
#if 0
	if (p->counter > current->counter)
	    need_resched = 1;
#endif
    }
#endif
    req->rq_dev = -1;
    req->rq_status = RQ_INACTIVE;
#ifdef MULTI_BH
    wake_up(&wait_for_request);
#endif
}
#endif

#ifdef DEVICE_INTR
#define CLEAR_INTR SET_INTR(NULL)
#else
#define CLEAR_INTR
#endif

#define INIT_REQUEST \
	if (!CURRENT) {\
		CLEAR_INTR; \
		return; \
	} \
	if (MAJOR(CURRENT->rq_dev) != MAJOR_NR) \
		panic("%s: request list destroyed (%d, %d)", DEVICE_NAME, MAJOR(CURRENT->rq_dev), MAJOR_NR); \
	if ((CURRENT->rq_bh) && (!buffer_locked(CURRENT->rq_bh))) { \
			panic("%s:block not locked", DEVICE_NAME); \
	}

#endif
