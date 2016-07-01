#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/config.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#include <arch/io.h>
#include <arch/keyboard.h>

#ifdef CONFIG_SIBO_CONSOLE_DIRECT

extern struct tty ttys[];

#define ESC 27

#define LSHIFT 	0x0100
#define RSHIFT 	0x0400
#define CTRL 	0x0200
#define ALT 	0x0800		/* Diamond key */
#define MENU	0x1000
#define PSION   0x8000

#define ANYSHIFT (LSHIFT|RSHIFT)

void AddQueue(unsigned char Key);

int Current_VCminor = 0;

/* for key repeat */
int last_key, key_repeat = 0;

/* from console.c */
extern char power_state;

int KeyboardInit(void)
{
/* Do nothing */
}

/*
 *  Psion keyboard is very different to PC, this is a quick stab to get
 *  something going. Simon Wood 12th June 1999
 */

void keyboard_irq(int irq, struct pt_regs *regs, void *data)
{
    int modifiers = psiongetchar();
    int key = (modifiers & 0x00FF);

    /* no key pressed */
    if (key == 0) {
	key_repeat = 0;
	last_key = 0;
	return;
    }

    if (key == last_key) {
	if (key_repeat < 5) {
	    key_repeat++;
	    return;
	}
    }

    last_key = key;
    key_repeat = 0;

    if (modifiers & PSION) {
	if (key == 0x31 && power_state == 1)	/* Psion + 1 */
	    power_off();

	if (key == 0x20 && power_state == 0)	/* Psion + Space */
	    power_on();

	return;
    }

    if (power_state == 0)
	return;			/* prevent key presses when off */

    if (modifiers & MENU) {
	if (key < 0x40 && key > 0x30) {	/* MENU + 1..9 */

#ifdef CONFIG_SIBO_VIRTUAL_CONSOLE

	    Console_set_vc(key - 0x31);

#else

	    AddQueue(ESC);
	    AddQueue(key - 0x31 + 'a');

#endif

	    return;
	}
    }

    switch (key) {
    case 0x18:			/* Arrow up */
	AddQueue(ESC);
	AddQueue('A');
	return;
    case 0x19:			/* Arrow down */
	AddQueue(ESC);
	AddQueue('B');
	return;
    case 0x1A:			/* Arrow right */
	AddQueue(ESC);
	AddQueue('C');
	return;
    case 0x1B:			/* Arrow left */
	AddQueue(ESC);
	AddQueue('D');
	return;
    default:
	AddQueue(key);
	return;
    }
}

/* Ack.  We can't add a character until the queue's ready
 */

void AddQueue(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    chq_addch(&ttyp->inq, Key);
}

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress(void)
{
    return chq_wait_rd(&ttys[0].inq, 0);
}

#endif
