/* chqueue.h (C) 1997 Chad Page */

#ifndef LX86_LINUXMT_CHQ_H
#define LX86_LINUXMT_CHQ_H

struct ch_queue {
    char		*base;
    struct wait_queue	wait;
    int 		size, start, len;
};

extern void chq_init(register struct ch_queue *,unsigned char *,int);
/*extern void chq_erase(register struct ch_queue *);*/
extern int chq_wait_wr(register struct ch_queue *,int);
extern int chq_addch(register struct ch_queue *,unsigned char);
extern int chq_delch(register struct ch_queue *);
/*extern int chq_peekch(register struct ch_queue *);*/
/*extern int chq_full(register struct ch_queue *);*/

extern int chq_wait_rd(register struct ch_queue *,int);
extern int chq_getch(register struct ch_queue *);

#endif
