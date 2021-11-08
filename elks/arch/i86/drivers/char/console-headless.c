/*
 * Headless Console
 *
 * Uses conio API to poll, receive and send characters
 *
 * Oct 2020 Greg Haerr
 */
#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include "console.h"
#include "conio.h"


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
#ifdef CONFIG_ARCH_PC98
    early_putchar((int) Ch);
#else
    if (Ch == '\n')
	conio_putc('\r');
    conio_putc(Ch);
#endif
}

void bell(void)
{
#ifndef CONFIG_ARCH_PC98
    conio_putc(7);	/* send ^G */
#endif
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
#ifdef CONFIG_ARCH_PC98
    int tty_out;
    int tvram_x;
    static int esc_seq = 0;
    static int esc_num = 0;
#endif

    while (tty->outq.len > 0) {
#ifdef CONFIG_ARCH_PC98
	tty_out = tty_outproc(tty);
	if (tty_out == 0x1B)
	    esc_seq = 1;
	else if ((tty_out == '[') && (esc_seq == 1)) {
	    esc_seq = 2;
	}
	else if ((tty_out >= 0x30) && (tty_out <= 0x39) && (esc_seq == 2)) {
	    esc_num *= 10;
	    esc_num += (tty_out-0x30);
	}
	else if ((tty_out > 0x40) && (esc_seq == 2)) {
	    esc_seq = 3;
	    if (tty_out == 'C') {
	        tvram_x = read_tvram_x();
	        tvram_x += esc_num * 2;
	        write_tvram_x(tvram_x);
	    }
	    else if (tty_out == 'K') {
	        clear_tvram();
	    }
	}
	else {
	    esc_seq = 0;
	    esc_num = 0;
    }

	if (!esc_seq) {
	    early_putchar(tty_out);
	}
#else
	conio_putc((byte_t)tty_outproc(tty));
#endif
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
