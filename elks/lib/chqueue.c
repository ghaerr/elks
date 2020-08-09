/* lib/chqueue.c
 * (C) 1997 Chad Page
 *
 * (Based on the original character queue code by Alan Cox(?))
 *
 * Array queue handling for new tty drivers... rather generic routine, and
 * flexible to boot.  The character queue uses a data structure that
 * references a seperately set up char array, removing hard limits from the
 * queing code.
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
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>

void chq_init(register struct ch_queue *q, unsigned char *buf, int size)
{
    debug3("CHQ: chq_init(%d, %d, %d)\n", q, buf, size);
    q->base = buf;
    q->size = size;
    q->len = q->start = 0;
}

int chq_wait_wr(register struct ch_queue *q, int nonblock)
{
    if (q->len == q->size) {
	if (nonblock)
	    return -EAGAIN;
	else {
	    interruptible_sleep_on(&q->wait);
	    if (q->len == q->size)
		return -EINTR;
	}
    }
    return 0;
}

/* Adds character c if there is room (or otherwise throws out new char) */
void chq_addch(register struct ch_queue *q, unsigned char c)
{
    debug5("CHQ: chq_addch(%d, %c, %d) q->len=%d q->start=%d\n", q, c, 0,
	   q->len, q->start);

    clr_irq();
    if (q->len < q->size) {
	q->base[(unsigned int)((q->start + q->len) & (q->size - 1))] = c;
	q->len++;
	set_irq();
	wake_up(&q->wait);
    } else set_irq();
}

int chq_wait_rd(register struct ch_queue *q, int nonblock)
{
    int	res = 0;

    pre_wait_interruptible(&q->wait);
    if (!q->len) {
	if (nonblock)
	    res = -EAGAIN;
	else {
	    wait();
	    if (!q->len)
		res = -EINTR;
	}
    }
    post_wait(&q->wait);
    return res;
}

/* Return first character in queue*/
int chq_getch(register struct ch_queue *q)
{
    int retval;

    debug4("CHQ: chq_getch(%d) q->len=%d q->start=%d q->size=%d\n",
	   q, q->len, q->start, q->size);

    if (!q->len)
	return -EAGAIN;
    else {
	clr_irq();
	retval = q->base[q->start];
	q->start++;
	q->start &= q->size - 1;
	q->len--;
	set_irq();
    }
    return retval;
}

int chq_peekch(struct ch_queue *q)
{
    return (q->len != 0);
}

#if 0
/* Deletes last character in list */
int chq_delch(register struct ch_queue *q)
{
    if (q->len == q->size) {
	q->len--;
	return 1;
    }
    return 0;
}

void chq_erase(register struct ch_queue *q)
{
    q->len = q->start = 0;
}

int chq_full(register struct ch_queue *q)
{
    return (q->len == q->size);
}
#endif
