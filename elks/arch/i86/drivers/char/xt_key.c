/*
 * Tables from the Minix book, as that's all I have on XT keyboard controllers.
 * They need be loadable, this doesn't look good on my Finnish keyboard.
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
#include <arch/io.h>
#include <arch/keyboard.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/config.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

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

#define ANYSHIFT 3		/* [LR]SHIFT */

/*************************************************************************
 * Queue for input received but not yet read by the application. SA 1996 *
 * There needs to be many buffers if we implement virtual consoles...... *
 *************************************************************************/

static void SetLeds();

void AddQueue(unsigned char Key);

int KeyboardInit(void)
{
/* Do nothing */
}

void xtk_init(void)
{
    KeyboardInit();
}

/*
 *	XT style keyboard I/O is almost civilised compared
 *	with the monstrosity AT keyboards became.
 */

void keyboard_irq(
     int irq,
     struct pt_regs *regs,
     void *dev_id)
{
    static unsigned ModeState = 0;
    static int E0Prefix = 0;
    int code, mode, IsRelease, key, E0 = 0;

    code = inb_p(KBD_IO);
    mode = inb_p(KBD_CTL);
    outb_p(mode | 0x80, KBD_CTL);	/* Necessary for the XT. */
    outb_p(mode, KBD_CTL);

    if (kraw) {
	AddQueue(code);
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
    IsRelease = code & 0x80;
    switch (code & 0x7F) {
    case 29:
	IsRelease ? (ModeState &= ~CTRL) : (ModeState |= CTRL);
	return;
    case 42:
	IsRelease ? (ModeState &= ~LSHIFT) : (ModeState |= LSHIFT);
	return;
    case 54:
	IsRelease ? (ModeState &= ~RSHIFT) : (ModeState |= RSHIFT);
	return;
    case 56:

#if defined(CONFIG_KEYMAP_DE) || defined(CONFIG_KEYMAP_SE)

	if (E0 == 0) {
	    IsRelease ? (ModeState &= ~ALT) : (ModeState |= ALT);
	} else {
	    IsRelease ? (ModeState &= ~ALT_GR) : (ModeState |= ALT_GR);
	};

#else

	IsRelease ? (ModeState &= ~ALT) : (ModeState |= ALT);

#endif

	return;
    case 58:
	ModeState ^= IsRelease ? 0 : CAPS;
	return;
    case 69:
	ModeState ^= IsRelease ? 0 : NUM;
	return;
    default:
	if (IsRelease)
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
	key = xtkb_scan_caps[code];
    else if (ModeState & ANYSHIFT && !(ModeState & CAPS))
	key = xtkb_scan_shifted[code];

    /* added for belgian keyboard (Stefke) */

    else if ((ModeState & CTRL) && (ModeState & ALT))
	key = xtkb_scan_ctrl_alt[code];

    /* end belgian                                  */

    /* added for German keyboard (Klaus Syttkus) */

    else if (ModeState & ALT_GR)
	key = xtkb_scan_ctrl_alt[code];
    /* end German */
    else
	key = xtkb_scan[code];

    if (ModeState & CTRL && code < 14 && !(ModeState & ALT))
	key = xtkb_scan_shifted[code];
    if (code < 70 && ModeState & NUM)
	key = xtkb_scan_shifted[code];
    /*
     *      Apply special modifiers
     */
    if (ModeState & ALT && !(ModeState & CTRL))	/* Changed to support CTRL-ALT */
	key |= 0x80;		/* META-.. */
    if (!key)			/* non meta-@ is 64 */
	key = '@';
    if (ModeState & CTRL && !(ModeState & ALT))	/* Changed to support CTRL-ALT */
	key &= 0x1F;		/* CTRL-.. */
    if (code < 0x45 && code > 0x3A) {	/* F1 .. F10 */

#ifdef CONFIG_CONSOLE_DIRECT

	if (ModeState & ALT) {
	    Console_set_vc(code - 0x3B);
	    return;
	}

#endif

	AddQueue(ESC);
	AddQueue(code - 0x3B + 'a');
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
    if (key == '\r')
	key = '\n';
    AddQueue(key);
}

/* Ack.  We can't add a character until the queue's ready */

void AddQueue(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    if (!tty_intcheck(ttyp, Key) && (ttyp->inq.size != 0))
	chq_addch(&ttyp->inq, Key, 0);
}

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress(void)
{
    isti();
    return chq_getch(&ttys[0].inq, 0, 1);
}

#endif
