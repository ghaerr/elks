/*
 * BIOS polling keyboard
 *
 * INT 16h functions 0,1 (keyboard)
 */
#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/timer.h>
#include <linuxmt/sched.h>
#include "console.h"
#include "bios-asm.h"

char kbd_name[] = "bios";

static void restart_timer(void);

/*
 * BIOS Keyboard Decoder
 */
static void kbd_timer(int data)
{
    int dav, extra = 0;

    if ((dav = bios_kbd_poll())) {
	if (dav & 0xFF)
	    Console_conin(dav & 0x7F);
	else {
	    dav = (dav >> 8) & 0xFF;
#ifndef CONFIG_CONSOLE_HEADLESS
	    if (dav >= 0x3B && dav <= 0x3D) {	/* temp console switch on F1-F3*/
		Console_set_vc(dav - 0x3B);
		dav = 0;
	    }
	    else if ((dav >= 0x68) && (dav < 0x6B)) {	/* Change VC */
		Console_set_vc((unsigned)(dav - 0x68));
		dav = 0;
	    }
	    else
#endif
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

    init_timer(&timer);
    timer.tl_expires = jiffies + 8;	/* every 8/100 second*/
    timer.tl_function = kbd_timer;
    add_timer(&timer);
}


void kbd_init(void)
{
    enable_irq(1);		/* enable BIOS Keyboard interrupts */
    restart_timer();
}
