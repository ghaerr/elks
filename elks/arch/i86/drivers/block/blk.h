#ifndef _BLK_H
#define _BLK_H

#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/kdev_t.h>
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
    unsigned char rq_cmd;	/* READ or WRITE */
    unsigned char rq_status;
    block32_t rq_blocknr;
    char *rq_buffer;
    ramdesc_t rq_seg;		/* L2 main/xms buffer segment */
    struct buffer_head *rq_bh;
    struct request *rq_next;

#ifdef BLOAT_FS
/* This may get used for dealing with waiting for requests later*/
    struct task_struct *rq_waiting;
    unsigned int rq_nr_sectors;
    unsigned int rq_current_nr_sectors;
#endif
};

#define RQ_INACTIVE	0
#define RQ_ACTIVE	1

/*
 * This is used in the elevator algorithm: Note that reads always go before
 * writes. This is natural: reads are much more time-critical than writes.
 *
 * Update: trying with writes being preferred due to test
 * by Alessandro Rubini..
 */

#define IN_ORDER(s1,s2) \
((s1)->rq_cmd > (s2)->rq_cmd || ((s1)->rq_cmd == (s2)->rq_cmd && \
((s1)->rq_dev < (s2)->rq_dev || (((s1)->rq_dev == (s2)->rq_dev && \
(s1)->rq_blocknr < (s2)->rq_blocknr)))))

struct blk_dev_struct {
    void (*request_fn) ();
    struct request *current_request;
};

/* For bioshd.c, idequery.c */
struct drive_infot {            /* CHS per drive*/
    int cylinders;
    int sectors;
    int heads;
    int sector_size;
    int fdtype;                 /* floppy fd_types[] index  or -1 if hd */
};
extern struct drive_infot *last_drive;	/* set to last drivep-> used in read/write */

extern struct blk_dev_struct blk_dev[MAX_BLKDEV];
extern void resetup_one_dev(struct gendisk *dev, int drive);

#ifdef MAJOR_NR

/*
 * Add entries as needed. Current block devices are
 * hard-disks, floppies, SSD and ramdisk.
 */

#ifdef RAMDISK

/* ram disk */
#define DEVICE_NAME "rd"
#define DEVICE_REQUEST do_rd_request
#define DEVICE_NR(device) ((device) & 7)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef SSDDISK

/* solid-state disk */
#define DEVICE_NAME "ssd"
#define DEVICE_REQUEST do_ssd_request
#define DEVICE_NR(device) ((device) & 3)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef FLOPPYDISK

static void floppy_on();	/*(unsigned int nr); */
static void floppy_off();	/*(unsigned int nr); */

#define DEVICE_NAME "fd"
#define DEVICE_INTR do_floppy
#define DEVICE_REQUEST do_fd_request
#define DEVICE_NR(device) ((device) & 3)
#define DEVICE_ON(device) floppy_on(DEVICE_NR(device))
#define DEVICE_OFF(device) floppy_off(DEVICE_NR(device))

#endif

#ifdef ATDISK

#define DEVICE_NAME "hd"
#define DEVICE_REQUEST do_directhd_request
#define DEVICE_NR(device) (MINOR(device)>>6)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef BIOSDISK

#define DEVICE_NAME "bioshd"
#define DEVICE_REQUEST do_bioshd_request
#define DEVICE_NR(device) (MINOR(device)>>MINOR_SHIFT)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef METADISK

#define DEVICE_NAME "udd"
#define DEVICE_REQUEST do_meta_request
#define DEVICE_NR(device) (MINOR(device))
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#define CURRENT		(blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV	DEVICE_NR(CURRENT->rq_dev)

static void (DEVICE_REQUEST) ();

static void end_request(int uptodate)
{
    register struct request *req;
    register struct buffer_head *bh;

    req = CURRENT;

    if (!uptodate) {
	printk("%s: I/O error: ", DEVICE_NAME);
	printk("dev %x, block %lu\n", req->rq_dev, req->rq_blocknr);

#ifdef MULTI_BH
#ifdef BLOAT_FS
	req->rq_nr_sectors--;
	req->rq_nr_sectors &= ~2;	/* 1K block size, 512 byte sector*/
#endif
	req->rq_blocknr++;

#endif
    }

    bh = req->rq_bh;

#ifdef BLOAT_FS
    req->rq_bh = bh->b_reqnext;
    bh->b_reqnext = NULL;
#endif

    mark_buffer_uptodate(bh, uptodate);
    unlock_buffer(bh);

#ifdef BLOAT_FS
    if ((bh = req->rq_bh) != NULL) {
	req->rq_current_nr_sectors = bh->b_size >> 9;
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
    struct task_struct *p;
    if ((p = req->rq_waiting) != NULL) {
	req->rq_waiting = NULL;
	p->state = TASK_RUNNING;
	/*if (p->counter > current->counter)
	    need_resched = 1;*/
    }
#endif

    req->rq_dev = -1;
    req->rq_status = RQ_INACTIVE;
#ifdef MULTI_BH
    wake_up(&wait_for_request);
#endif
}
#endif /* MAJOR_NR */

#define INIT_REQUEST(req) \
	if (!req || req->rq_dev < 0) \
		return; \
	if (MAJOR(req->rq_dev) != MAJOR_NR) \
		panic("%s: request list destroyed (%d, %d)", \
			DEVICE_NAME, MAJOR(req->rq_dev), MAJOR_NR); \
	if (req->rq_bh && !EBH(req->rq_bh)->b_locked) \
		panic("%s:block not locked", DEVICE_NAME); \

#endif
