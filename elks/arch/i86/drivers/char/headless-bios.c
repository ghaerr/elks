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

static void restart_timer(void);

/* called by scheduler to poll BIOS keyboard*/
static void kbd_timer(int data)
{
    int dav, extra;

    if ((dav = bios_kbd_poll())) {
	if (dav & 0xFF)
	    Console_conin(dav & 0x7F);
	else {
	    dav = (dav >> 8) & 0xFF;
	    if (dav >= 0x3B && dav < 0x45)	/* Function keys */
		dav = dav - 0x3B + 'a';
	    else {
		switch(dav) {
		case 0x48: dav = 'A'; break;	/* up*/
		case 0x50: dav = 'B'; break;	/* down*/
		case 0x4d: dav = 'C'; break;	/* right*/
		case 0x4b: dav = 'D'; break;	/* left*/
		case 0x47: dav = 'H'; break;	/* home*/
		case 0x4f: dav = 'F'; break;	/* end*/
		case 0x49: dav = '5'; extra = '~'; break; /* pgup*/
		case 0x51: dav = '6'; extra = '~'; break; /* pgdn*/
		default:   dav = 0;
		}
	    }
	    if (dav) {
		Console_conin(033);
		Console_conin('[');
		Console_conin(dav);
		if (extra)
		    Console_conin(extra);
	    }
	}
    }
    restart_timer();
}

/* initialize timer for scheduler*/
static void restart_timer(void)
{
    static struct timer_list timer;

    init_timer(&timer);
    timer.tl_expires = jiffies + 8;	/* every 8/100 second*/
    timer.tl_function = kbd_timer;
    add_timer(&timer);
}

void console_init(void)
{
    enable_irq(1);		/* enable BIOS Keyboard interrupts */
    restart_timer();
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
