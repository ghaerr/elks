/* Tables from the Minix book, as that's all I have on XT keyboard
 * controllers. They need to be loadable, this doesn't look good on
 * my Finnish keyboard.
 *
 ***************************************************************
 * Added primitive buffering, and function stubs for vfs calls *
 * Removed vfs funcs, they belong better to the console driver *
 * Saku Airila 1996                                            *
 ***************************************************************
 * Changed code to work with belgian keyboard                  *
 * Stefaan (Stefke) Van Dooren 1998                            *
 ***************************************************************/

#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/config.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/signal.h>
#include <linuxmt/ntty.h>

#include <arch/io.h>
#include <arch/keyboard.h>

#ifdef CONFIG_CONSOLE_DIRECT

extern struct tty ttys[];

#define ESC 27
#define KB_SIZE 64

/*
 * Include the relevant keymap.
 */
#include "KeyMaps/keymaps.h"

int Current_VCminor = 0;
int kraw = 0;

/*
 *	Keyboard state - the poor little keyboard controller hasnt
 *	got the brains to remember itself.
 *
 *********************************************
 * Changed this a lot. Made it work, too. ;) *
 * SA 1996                                   *
 *********************************************/

#define LSHIFT 1
#define RSHIFT 2
#define CTRL 4
#define ALT 8
#define CAPS 16
#define NUM 32
#define ALT_GR 64

#define ANYSHIFT (LSHIFT | RSHIFT)

/* Ack.  We can't add a character until the queue's ready
 */

void AddQueue(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    if (!tty_intcheck(ttyp, Key) && (ttyp->inq.size != 0))
	chq_addch(&ttyp->inq, Key, 0);
}

/*************************************************************************
 * Queue for input received but not yet read by the application. SA 1996 *
 * There needs to be many buffers if we implement virtual consoles...... *
 *************************************************************************/

int KeyboardInit(void)
{
    /* Do nothing */ ;
}

void xtk_init(void)
{
    KeyboardInit();
}

/*
 *	XT style keyboard I/O is almost civilised compared
 *	with the monstrosity AT keyboards became.
 */

void keyboard_irq(int irq, struct pt_regs *regs, void *dev_id)
{
    static unsigned int ModeState = 0;
    static int E0Prefix = 0;
    int code, mode, E0 = 0;
    register char *keyp;
    register char *IsReleasep;

    code = inb_p((void *) KBD_IO);
    mode = inb_p((void *) KBD_CTL);

    /* Necessary for the XT. */
    outb_p((unsigned char) (mode | 0x80), (void *) KBD_CTL);
    outb_p((unsigned char) mode, (void *) KBD_CTL);

    if (kraw) {
	AddQueue((unsigned char) code);
	return;
    }
    if (code == 0xE0) {		/* Remember this has been received */
	E0Prefix = 1;
	return;
    }
    if (E0Prefix) {
	E0 = 1;
	E0Prefix = 0;
    }
    IsReleasep = (char *)(code & 0x80);
    switch (code & 0x7F) {
    case 29:
	IsReleasep ? (ModeState &= ~CTRL) : (ModeState |= CTRL);
	return;
    case 42:
	IsReleasep ? (ModeState &= ~LSHIFT) : (ModeState |= LSHIFT);
	return;
    case 54:
	IsReleasep ? (ModeState &= ~RSHIFT) : (ModeState |= RSHIFT);
	return;
    case 56:

#if defined(CONFIG_KEYMAP_DE) || defined(CONFIG_KEYMAP_SE)

	if (E0 == 0) {
	    IsReleasep ? (ModeState &= ~ALT) : (ModeState |= ALT);
	} else {
	    IsReleasep ? (ModeState &= ~ALT_GR) : (ModeState |= ALT_GR);
	}

#else

	IsReleasep ? (ModeState &= ~ALT) : (ModeState |= ALT);

#endif

	return;
    case 58:
	ModeState ^= IsReleasep ? 0 : CAPS;
	return;
    case 69:
	ModeState ^= IsReleasep ? 0 : NUM;
	return;
    default:
	if (IsReleasep)
	    return;
	break;
    }

    /*      Handle CTRL-ALT-DEL     */

    if ((code == 0x53) && (ModeState & CTRL) && (ModeState & ALT))
	ctrl_alt_del();

    /*
     *      Pick the right keymap
     */
    if (ModeState & CAPS && !(ModeState & ANYSHIFT))
	keyp = (char *) xtkb_scan_caps[code];
    else if (ModeState & ANYSHIFT && !(ModeState & CAPS))
	keyp = (char *) xtkb_scan_shifted[code];

    /* added for belgian keyboard (Stefke) */

    else if ((ModeState & CTRL) && (ModeState & ALT))
	keyp = (char *) xtkb_scan_ctrl_alt[code];

    /* end belgian                                  */

    /* added for German keyboard (Klaus Syttkus) */

    else if (ModeState & ALT_GR)
	keyp = (char *) xtkb_scan_ctrl_alt[code];
    /* end German */
    else
	keyp = (char *) xtkb_scan[code];

    if (ModeState & CTRL && code < 14 && !(ModeState & ALT))
	keyp = (char *) xtkb_scan_shifted[code];
    if (code < 70 && ModeState & NUM)
	keyp = (char *) xtkb_scan_shifted[code];
    /*
     *      Apply special modifiers
     */
    if (ModeState & ALT && !(ModeState & CTRL))	/* Changed to support CTRL-ALT */
	keyp = (char *)(((int) keyp) | 0x80); /* META-.. */
    if (!keyp)			/* non meta-@ is 64 */
	keyp = (char *) '@';
    if (ModeState & CTRL && !(ModeState & ALT))	/* Changed to support CTRL-ALT */
	keyp = (char *)(((int) keyp) & 0x1F); /* CTRL-.. */
    if (code < 0x45 && code > 0x3A) {	/* F1 .. F10 */

#ifdef CONFIG_CONSOLE_DIRECT

	if (ModeState & ALT) {
	    Console_set_vc((unsigned) (code - 0x3B));
	    return;
	}
#endif

	AddQueue(ESC);
	AddQueue((unsigned char) (code - 0x3B + 'a'));
	return;
    }
    if (E0)			/* Is extended scancode */
	switch (code) {
	case 0x48:		/* Arrow up */
	    AddQueue(ESC);
	    AddQueue('A');
	    return;
	case 0x50:		/* Arrow down */
	    AddQueue(ESC);
	    AddQueue('B');
	    return;
	case 0x4D:		/* Arrow right */
	    AddQueue(ESC);
	    AddQueue('C');
	    return;
	case 0x4B:		/* Arrow left */
	    AddQueue(ESC);
	    AddQueue('D');
	    return;
	case 0x1c:		/* keypad enter */
	    AddQueue('\n');
	    return;
	}
    if (((int)keyp) == '\r')
	keyp = (char *) '\n';
    AddQueue((unsigned char) keyp);
}

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress(void)
{
    set_irq();
    return chq_getch(&ttys[0].inq, 0, 1);
}

#endif
