/* chqueue.h
 * (C) 1997 Chad Page
 */
#ifndef _LINUXMT_CHQ_H_
#define _LINUXMT_CHQ_H_

struct ch_queue {
	char *buf;
	int size, tail, len;
	struct wait_queue wq;
};

int chq_init();
int chq_erase();
int chq_addch();
int chq_delch();
int chq_getch();
int chq_peekch();

#endif /* _LINUXMT_CHQ_H_ */
