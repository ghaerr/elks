/* lib/chqueue.c
 * (C) 1997 Chad Page 
 *
 * (Based on the original character queue code by Alan Cox(?))
 *
 * Array queue handling for new tty drivers... rather generic routine, and
 * flexible to boot.  The character queue uses a data structure which 
 * references a seperately set up char array, removing hard limits from the
 * queing code
 *
 * I can't help but think there's a race condition somewhere in here :)
 *
 * 'warming' will be done by the tty driver itself when it copies data over
 *
 * 16 July 2001 : A divide is quite expensive on an 8086 so I changed the 
 * code to use logical addition and not modulo. The only drawback is we
 * have to use only power of two buffer sizes. (Harry Kalogirou)
 */

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/debug.h>
#include <linuxmt/types.h>

int chq_erase(register struct ch_queue *q)
{
    q->len = q->tail = 0;
}

int chq_init(register struct ch_queue *q, unsigned char *buf, int size)
{
    printd_chq3("CHQ: chq_init(%d, %d, %d)\n", q, buf, size);
    q->buf = buf;
    q->size = size;
    chq_erase(q);
}

/* Adds character c, waiting if wait=1 (or otherwise throws out new char) */
int chq_addch(register struct ch_queue *q, unsigned char c, int wait)
{
    unsigned int nhead;

    printd_chq5("CHQ: chq_addch(%d, %c, %d) q->len=%d q->tail=%d\n", q, c, 0,
		q->len, q->tail);

    if (q->len == q->size) {
	if (wait) {
	    printd_chq("CHQ: addch sleeping\n");
	    interruptible_sleep_on(&q->wq);
	    printd_chq("CHQ: addch waken up\n");
	}
    }

    if (q->len == q->size)
	return -1;

    nhead = (q->tail + q->len) & (q->size - 1);
    q->buf[nhead] = c;
    q->len++;
    wake_up(&q->wq);

    return 0;
}

/* Deletes last character in list */
int chq_delch(register struct ch_queue *q)
{
    if (q->len == q->size) {
	q->len--;
	return 1;
    } else
	return 0;
}

/* Gets tail character, waiting for one if wait != 0 */
int chq_getch(register struct ch_queue *q, register unsigned char *c, int wait)
{
    int ntail, retval;

    printd_chq6("CHQ: chq_getch(%d, %d, %d) q->len=%d q->tail=%d q->size=%d\n",
		q, c, wait, q->len, q->tail, q->size);

    if (q->len == 0) {
	if (wait) {
	    printd_chq("CHQ: getch sleeping\n");
	    interruptible_sleep_on(&q->wq);
	    printd_chq("CHQ: getch wokeup\n");
	}
    }
    if (q->len == 0)
	return -1;

    retval = q->buf[q->tail];
    q->tail++;
    q->tail &= q->size - 1;
    q->len--;

    wake_up(&q->wq);
    if (c != 0)
	*c = retval;

    return retval;
}

int chq_peekch(register struct ch_queue *q)
{
    return (q->len != 0);
}

int chq_full(register struct ch_queue *q)
{
    return (q->len == q->size);
}
