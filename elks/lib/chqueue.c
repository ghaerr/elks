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
 *
 * Oct 2020: No longer require power of two buffer sizes. (Greg Haerr)
 */

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <arch/irq.h>

void chq_init(register struct ch_queue *q, unsigned char *buf, int size)
{
    debug("CHQ: chq_init(%d, %d, %d)\n", q, buf, size);
    q->base = buf;
    q->size = size;
    q->len = q->head = q->tail = 0;
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
    debug("CHQ: chq_addch(%d, %c) q->len=%d q->head=%d\n", q, c,
	   q->len, q->head);

    clr_irq();
    if (q->len < q->size) {
	q->base[q->head] = c;
	if (++q->head >= q->size)
	    q->head = 0;
	q->len++;
	set_irq();
	wake_up(&q->wait);
    } else set_irq();
}

void chq_addch_nowakeup(register struct ch_queue *q, unsigned char c)
{
    clr_irq();
    if (q->len < q->size) {
	q->base[q->head] = c;
	if (++q->head >= q->size)
	    q->head = 0;
	q->len++;
    }
    set_irq();
}

int chq_wait_rd(register struct ch_queue *q, int nonblock)
{
    int	res = 0;

    prepare_to_wait_interruptible(&q->wait);
    if (!q->len) {
	if (nonblock)
	    res = -EAGAIN;
	else {
	    do_wait();
	    if (!q->len)
		res = -EINTR;
	}
    }
    finish_wait(&q->wait);
    return res;
}

/* Return first character in queue*/
int chq_getch(register struct ch_queue *q)
{
    int retval;

    debug("CHQ: chq_getch(%d) q->len=%d q->head=%d q->size=%d\n",
	   q, q->len, q->head, q->size);

    if (!q->len)
	return -EAGAIN;
    else {
	clr_irq();
	retval = q->base[q->tail];
	if (++q->tail >= q->size)
	    q->tail = 0;
	q->len--;
	set_irq();
    }
    return retval;
}

int chq_peekch(struct ch_queue *q)
{
    return (q->len != 0);
}

#if UNUSED
int chq_full(register struct ch_queue *q)
{
    return (q->len == q->size);
}
#endif
