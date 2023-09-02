#ifndef _BLK_H
#define _BLK_H

#include <linuxmt/config.h>
#include <linuxmt/limits.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/genhd.h>
#include <linuxmt/trace.h>

struct request {
    kdev_t rq_dev;              /* block device */
    unsigned char rq_cmd;       /* READ or WRITE */
    unsigned char rq_status;    /* RQ_INACTIVE or RQ_ACTIVE */
    sector_t rq_sector;         /* start device logical sector # */
    unsigned int rq_nr_sectors; /* multi-sector I/O # sectors */
    char *rq_buffer;            /* I/O buffer address */
    ramdesc_t rq_seg;           /* L1 or L2 ext/xms buffer segment */
    struct buffer_head *rq_bh;  /* system buffer head for notifications and locking */
    struct request *rq_next;    /* next request, used when async I/O */
};

#define RQ_INACTIVE     0
#define RQ_ACTIVE       1

/*
 * This is used in the elevator algorithm.  We don't prioritise reads
 * over writes any more --- although reads are more time-critical than
 * writes, by treating them equally we increase filesystem throughput.
 * This turns out to give better overall performance.  -- sct
 */

#define IN_ORDER(s1,s2) \
((s1)->rq_dev < (s2)->rq_dev || (((s1)->rq_dev == (s2)->rq_dev && \
(s1)->rq_sector < (s2)->rq_sector)))

struct blk_dev_struct {
    void (*request_fn) ();
    struct request *current_request;
};

/* For bioshd.c, idequery.c */
struct drive_infot {            /* CHS per drive*/
    unsigned int cylinders;
    int sectors;
    int heads;
    int sector_size;
    int fdtype;                 /* floppy fd_types[] index  or -1 if hd */
};
extern struct drive_infot *last_drive;  /* set to last drivep-> used in read/write */

extern unsigned char hd_drive_map[];

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
#define DEVICE_NR(device) ((device) & 1)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef SSDDISK

/* solid-state disk */
#define DEVICE_NAME "ssd"
#define DEVICE_REQUEST do_ssd_request
#define DEVICE_NR(device) ((device) & 0)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#endif

#ifdef FLOPPYDISK

static void floppy_on(unsigned int nr);
static void floppy_off(unsigned int nr);

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

#define CURRENT         (blk_dev[MAJOR_NR].current_request)
#define CURRENT_DEV     DEVICE_NR(CURRENT->rq_dev)

static void (DEVICE_REQUEST) ();

extern struct wait_queue wait_for_request;

static void end_request(int uptodate)
{
    struct request *req;
    struct buffer_head *bh;

    req = CURRENT;

    if (!uptodate) {
        printk(DEVICE_NAME ": I/O %s error dev %D sector %lu\n",
            (req->rq_cmd == WRITE)? "write": "read",
            req->rq_dev, req->rq_sector);
    }

#ifdef MULTI_BH
    int count = BLOCK_SIZE / get_sector_size(req->rq_dev);
    req->rq_nr_sectors -= count;
    req->rq_sector += count;
#endif

    bh = req->rq_bh;
    mark_buffer_uptodate(bh, uptodate);
    unlock_buffer(bh);

    DEVICE_OFF(req->dev);
    CURRENT = req->rq_next;
    req->rq_status = RQ_INACTIVE;

#ifdef CONFIG_ASYNCIO
    wake_up(&wait_for_request);
#endif
}
#endif /* MAJOR_NR */

#ifdef CHECK_BLOCKIO
#define CHECK_REQUEST(req) \
    if (req->rq_status != RQ_ACTIVE \
        || (req->rq_cmd != READ && req->rq_cmd != WRITE) \
        || MAJOR(req->rq_dev) != MAJOR_NR) \
        panic(DEVICE_NAME ": bad request dev %D cmd %d active %d", \
            req->rq_dev, req->rq_cmd, req->rq_status); \
    if (req->rq_bh && !EBH(req->rq_bh)->b_locked) \
        panic(DEVICE_NAME ": not locked");
#else
#define CHECK_REQUEST(req)
#endif

#endif /* _BLK_H */
