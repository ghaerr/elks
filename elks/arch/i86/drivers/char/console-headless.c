/*
 * Headless Console
 *
 * Uses conio API to poll, receive and send characters
 *
 * Oct 2020 Greg Haerr
 */
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include "console.h"
#include "conio.h"


void INITPROC console_init(void)
{
    kbd_init();
    printk("Headless console\n");
}

void Console_conin(unsigned char Key)
{
    register struct tty *ttyp = &ttys[0];	/* /dev/tty1*/

    if (!tty_intcheck(ttyp, Key))
	chq_addch(&ttyp->inq, Key);
}

void Console_conout(dev_t dev, int Ch)
{
    if (Ch == '\n')
	conio_putc('\r');
    conio_putc(Ch);
}

#ifndef CONFIG_ARCH_PC98
void bell(void)
{
    conio_putc(7);	/* send ^G */
}
#endif

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
	conio_putc(tty_outproc(tty));
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
