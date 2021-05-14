/*
 * Headless Console using 80C18X's Serial1
 *
 * Based off Greg Haerr's "headless-bios.c"
 * 
 * This file contains code used for the embedded 8018X family only.
 * 
 * 16 May 21 Santiago Hormazabal
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/mm.h>
#include <linuxmt/timer.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <arch/io.h>
#include <arch/segment.h>
#include "console.h"


void console_init(void)
{
    printk("8018X serial headless console\n");
}

void cout(char c)
{
    while((inw(0xff00 + 0x66) & 8) == 0); /* S0STS */
    outb(c, 0xff00 + 0x6A); /* T0BUF */
}

void Console_conin(unsigned char Key)
{
    register struct tty *ttyp = &ttys[0];	/* /dev/tty1*/

    if (!tty_intcheck(ttyp, Key))
	chq_addch(&ttyp->inq, Key);
}

void Console_conout(dev_t dev, char Ch)
{
    if (Ch == '\n')
        cout('\r');
    
    cout(Ch);
}

static int Console_ioctl(struct tty *tty, int cmd, char *arg)
{
    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
	return 0;
    }

    return -EINVAL;
}

static int Console_write(register struct tty *tty)
{
    int cnt = 0;

    while (tty->outq.len > 0) {
        cout((byte_t)tty_outproc(tty));
        cnt++;
    }
    return cnt;
}

static void Console_release(struct tty *tty)
{
    ttystd_release(tty);
}

static int Console_open(register struct tty *tty)
{
    return ttystd_open(tty);
}

struct tty_ops headlesscon_ops = {
    Console_open,
    Console_release,
    Console_write,
    NULL,
    Console_ioctl,
    Console_conout
};
