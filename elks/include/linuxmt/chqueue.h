/* chqueue.h (C) 1997 Chad Page */

#ifndef __LINUXMT_CHQ_H
#define __LINUXMT_CHQ_H

struct ch_queue {
    unsigned char	*base;
    int 		size, start, len;
    struct wait_queue	wait;
};

extern void chq_init(register struct ch_queue *,unsigned char *,int);
/*extern void chq_erase(register struct ch_queue *);*/
extern int chq_wait_wr(register struct ch_queue *,int);
extern void chq_addch(register struct ch_queue *,unsigned char);
extern void chq_addch_nowakeup(register struct ch_queue *,unsigned char);
extern int chq_delch(register struct ch_queue *);
extern int chq_peekch(register struct ch_queue *);
/*extern int chq_full(register struct ch_queue *);*/

extern int chq_wait_rd(register struct ch_queue *,int);
extern int chq_getch(register struct ch_queue *);

#endif
