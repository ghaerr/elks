/* lib/chqueue.c
 * (C) 1997 Chad Page 
 *
 * (Based on the original character queue code by Alan Cox(?)) */

/* Array queue handling for new tty drivers... rather generic routine, and
 * flexible to boot.  The character queue uses a data structure which 
 * references a seperately set up char array, removing hard limits from the
 * queing code */

/* I can't help but think there's a race condition somewhere in here :) */

/* 'warming' will be done by the tty driver itself when it copies data over */

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/debug.h>
#include <linuxmt/types.h>

int chq_init(q, buf, size)
register struct ch_queue * q;
unsigned char *buf; 
int size;
{
	printd_chq3("CHQ: chq_init(%d, %d, %d)\n", q, buf, size);
	q->buf = buf;
	q->len = 0;
	q->tail = 0;
	q->size = size;
	q->wq = NULL;
}

#if 0
int chq_erase(q)
register struct ch_queue * q;
{
	q->len = q->tail = 0;	
}
#endif

/* Adds character c, waiting if wait=1 (or otherwise throws out new char) */
int chq_addch(q, c /*, wait */)
register struct ch_queue * q;
unsigned char c;
/* int wait; */
{
	unsigned int nhead; 

	printd_chq5("CHQ: chq_addch(%d, %c, %d) q->len=%d q->tail=%d\n", q, c, 0, q->len, q->tail);

/* If waiting is required then the code below must be put back in */
/* And the wait argument put back */
#if 0
	if (q->len == q->size) {
		if (wait) {
			printd_chq("CHQ: addch sleeping\n");
			interruptible_sleep_on(&q->wq);
			printd_chq("CHQ: addch waken up\n");
		}
	}
#endif
	if (q->len == q->size) {
		return -1;
	}

	nhead = (q->tail + q->len) % q->size;	
	q->buf[nhead] = c; q->len++; 
	wake_up(&q->wq);	
	return 0;
}

#if 0
/* Deletes last character in list */ 
int chq_delch(q)
register struct ch_queue * q;
{
	if (q->len == q->size) {
		q->len--;
		return 1;
	}
	else
		return 0;
}
#endif

/* Gets tail character, waiting for one if wait != 0 */
int chq_getch(q, c, wait)
register struct ch_queue *q;
register unsigned char *c;
int wait;
{
	int ntail;
	int retval;

	printd_chq6("CHQ: chq_getch(%d, %d, %d) q->len=%d q->tail=%d q->size=%d\n", q, c, wait, q->len, q->tail, q->size);

	if (q->len == 0) {
		if (wait) {
			printd_chq("CHQ: getch sleeping\n");
			interruptible_sleep_on(&q->wq);	
			printd_chq("CHQ: getch wokeup\n");
		}
	}
	if (q->len == 0) {
		return -1;
	}

	retval = q->buf[q->tail];
	q->tail++;
	q->tail %= q->size;
	q->len--;

	wake_up(&q->wq);
	if (c != 0) *c = retval;
	return retval;	
}

int chq_peekch(q)
register struct ch_queue *q;
{
	if (q->len == 0)
		return 0;

	return q->buf[q->tail];
}
