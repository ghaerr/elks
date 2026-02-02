#ifndef __LINUXMT_CHQ_H
#define __LINUXMT_CHQ_H

/* chqueue.h (C) 1997 Chad Page, rewritten Greg Haerr Oct 2020 */

struct ch_queue {               /* NOTE: members used in fastser.S driver */
    int                 len;    /* 0 */
    int                 size;   /* 2 doesn't have to be power of two */
    int                 head;   /* 4 */
    int                 tail;
    unsigned char       *base;  /* 8 */
    struct wait_queue   wait;
};

extern void chq_init(register struct ch_queue *,unsigned char *,int);
extern int chq_wait_wr(register struct ch_queue *,int);
extern int chq_wait_rd(register struct ch_queue *,int);
extern void chq_addch(register struct ch_queue *,unsigned char);
extern void chq_addch_nowakeup(register struct ch_queue *,unsigned char);
extern int chq_peekch(register struct ch_queue *);
extern int chq_peek(register struct ch_queue *);
extern int chq_getch(register struct ch_queue *);
/*extern int chq_full(register struct ch_queue *);*/

#endif
