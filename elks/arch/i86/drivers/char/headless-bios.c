/*
 * Headless Console using two BIOS calls
 *
 * INT 16h functions 0,1 (keyboard)
 * INT 10h function 0x0E (write teletype)
 *
 * Oct 2020 Greg Haerr
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
#include "bios-asm.h"


void console_init(void)
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

void Console_conout(dev_t dev, char Ch)
{
    if (Ch == '\n')
	bios_textout('\r');
    bios_textout(Ch);
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
	bios_textout((byte_t)tty_outproc(tty));
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
