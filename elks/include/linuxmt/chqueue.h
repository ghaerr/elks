/* chqueue.h
 * (C) 1997 Chad Page
 */

struct ch_queue {
	char *buf;
	int size, tail, len;
/*	struct wait_queue wa_qu;
	struct wait_queue * wq; */
	struct wait_queue * wq;
};

int chq_init();
int chq_erase();
int chq_addch();
int chq_delch();
int chq_getch();
int chq_peekch();

