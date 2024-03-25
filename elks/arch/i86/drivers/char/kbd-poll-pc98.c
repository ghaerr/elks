/*
 * Polling keyboard driver
 *
 * Calls conio_poll to get kbd input
 */
#include <linuxmt/config.h>
#include <linuxmt/timer.h>
#include <linuxmt/sched.h>
#include "console.h"
#include "conio.h"

char kbd_name[] = "polling";

static void restart_timer(void);

/*
 * Poll for keyboard character
 * Decodes non-zero high byte into arrow/fn keys
 */
static void kbd_timer(int data)
{
    int dav, extra = 0;

    if ((dav = conio_poll())) {
	if (dav & 0xFF)
	    Console_conin(dav & 0x7F);
	else {
	    dav = (dav >> 8) & 0xFF;

	    if (dav & 0x4)
	        dav = 'A';	/* up*/
	    else if (dav & 0x20)
	        dav = 'B';	/* down*/
	    else if (dav & 0x10)
	        dav = 'C';	/* right*/
	    else if (dav & 0x8)
	        dav = 'D';	/* left*/
	    else if (dav & 0x80) {		/* Roll Down for PC-98 */
	        dav = '5';	/* pgup*/
	        extra = '~';
	    }
	    else if (dav & 0x40) {		/* Roll Up for PC-98 */
	        dav = '6';	/* pgdn*/
	        extra = '~';
	    }
	    else
	        dav = 0;

	    if (dav) {
		Console_conin(033);
#ifdef CONFIG_EMUL_ANSI
		Console_conin('[');
#endif
		Console_conin(dav);
#ifdef CONFIG_EMUL_ANSI
		if (extra)
		    Console_conin(extra);
#endif
	    }
	}
    }
    restart_timer();
}

static void restart_timer(void)
{
    static struct timer_list timer;

    timer.tl_expires = jiffies + (8 * HZ/100);	/* every 8/100 second*/
    timer.tl_function = kbd_timer;
    add_timer(&timer);
}


void kbd_init(void)
{
    conio_init();
    restart_timer();
}
