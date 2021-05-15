#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/timer.h>

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/param.h>
#include <arch/ports.h>

/*
 *	Timer tick routine
 *
 * 9/1999 The 100 Hz system timer 0 can configure for variable input
 *        frequency. Christian Mardm"oller (chm@kdt.de)
 */

volatile jiff_t jiffies = 0;
static int spin_on = 0;

extern void rs_pump(void);

void timer_tick(int irq, struct pt_regs *regs, void *data)
{
    do_timer(regs);

#ifdef CONFIG_CHAR_DEV_RS
    rs_pump();		/* check if received serial chars and call wake_up*/
#endif

#ifdef CONFIG_CONSOLE_DIRECT
    /* spin timer wheel in upper right of screen*/
    if (spin_on && !(jiffies & 7)) {
	static unsigned char wheel[4] = {'-', '\\', '|', '/'};
	static int c = 0;

	pokeb((79 + 0*80) * 2, 0xB800, wheel[c++ & 0x03]);
    }
#endif
}

void spin_timer(int onflag)
{
#ifdef CONFIG_CONSOLE_DIRECT
    if ((spin_on = onflag) == 0)
	pokeb((79 + 0*80) * 2, 0xB800, ' ');
#endif
}
