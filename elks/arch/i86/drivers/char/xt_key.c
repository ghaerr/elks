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

#define SSC 0xC0

static unsigned char tb_state[] = {
    SSC, CTRL, SSC, SSC,			/*1C->1F*/
    SSC, SSC, SSC, SSC, SSC, SSC, SSC, SSC,	/*20->27*/
    SSC, SSC, LSHIFT, SSC, SSC, SSC, SSC, SSC,	/*28->2F*/
    SSC, SSC, SSC, SSC, SSC, SSC, RSHIFT, SSC,	/*30->37*/
    SSC, SSC, CAPS,				/*38->3A*/
    'a', 'b', 'c', 'd', 'e',			/*3B->3F, Function Keys*/
    'f', 'g', 'h', 'i', 'j',			/*40->44, Function Keys*/
    NUM, SSC, SSC,				/*45->47*/
    0xB7, SSC, SSC, 0xBA, SSC, 0xB9, SSC, SSC,	/*48->4F*/
    0xB8, SSC, SSC, SSC, SSC, SSC, ALT, SSC,	/*50->57*/
};

static unsigned char state_code[] = {
    0,	/* All status are 0 */
    1,	/* SHIFT */
    0,	/* CTRL */
    1,	/* SHIFT CTRL */
    0,	/* ALT */
    1,	/* SHIFT ALT */
    3,	/* CTRL ALT */
    1,	/* SHIFT CTRL ALT */
    2,	/* CAPS */
    0,	/* CAPS SHIFT */
    2,	/* CAPS CTRL */
    0,	/* CAPS SHIFT CTRL */
    2,	/* CAPS ALT */
    0,	/* CAPS SHIFT ALT */
    2,	/* CAPS CTRL ALT */
    3,	/* CAPS SHIFT CTRL ALT */
};
static unsigned char *scan_tabs[] = {
    xtkb_scan,
    xtkb_scan_shifted,
    xtkb_scan_caps,
    xtkb_scan_ctrl_alt,
};

/* Ack.  We can't add a character until the queue's ready
 */

void AddQueue(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    if (!tty_intcheck(ttyp, Key))
	chq_addch(&ttyp->inq, Key, 0);
}

/*************************************************************************
 * Queue for input received but not yet read by the application. SA 1996 *
 * There needs to be many buffers if we implement virtual consoles...... *
 *************************************************************************/

void xtk_init(void)
{
    /* Set off the initial keyboard interrupt handler */

    if (request_irq(1, keyboard_irq, NULL))
	panic("Unable to get keyboard");
}

/*
 *	XT style keyboard I/O is almost civilised compared
 *	with the monstrosity AT keyboards became.
 */

void keyboard_irq(int irq, struct pt_regs *regs, void *dev_id)
{
    static unsigned int ModeState = 0;
    static int E0Prefix = 0;
    int code, mode;
    register char *keyp_E0 = (char *)0;	/*[char *keyp; int E0]*/
    register char *IsReleasep;

    code = inb_p((void *) KBD_IO);

    /* Necessary for the XT. */
    mode = inb_p((void *) KBD_CTL);
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
	keyp_E0 = (char *)1;	/*[ E0 ]*/
	E0Prefix = 0;
    }
    IsReleasep = (char *)(code & 0x80);
    code &= 0x7F;
    /*
     * Clasify scancode such that
     *  mode = 00xx xxxxB, Status key
     *         01xx xxxxB, Function key
     *         10xx xxxxB, Control key
     *         11xx xxxxB, Simple Scan Code
     */
    mode = (code >= 0x1C) ? tb_state[code - 0x1C] : SSC;

    /* --------------Process status keys-------------- */

    if(!(mode & 0xC0)) {
#if defined(CONFIG_KEYMAP_DE) || defined(CONFIG_KEYMAP_SE)
	if((mode == ALT) && ((int)keyp_E0 != 0))	/*[ E0 ]*/
	    mode = ALT_GR;
#endif
	IsReleasep ? (ModeState &= ~mode) : (ModeState |= mode);
	return;
    }
    if(IsReleasep)
	return;

    switch(mode & 0xC0) {
    case 0x40:	/* F1 .. F10 */
    /* --------------Handle Function keys-------------- */

#ifdef CONFIG_CONSOLE_DIRECT

	if (ModeState & ALT) {
	    Console_set_vc((unsigned) (code - 0x3B));
	    return;
	}
#endif

	AddQueue(ESC);
	AddQueue((unsigned char)mode);
	return;

    /* --------------Handle extended scancodes-------------- */
    case 0x80:
	if((int)keyp_E0) {		/* Is extended scancode? */	/*[ E0 ]*/
	    mode &= 0x3F;
	    if(mode) {
		AddQueue(ESC);
#ifdef CONFIG_EMUL_ANSI
		AddQueue('[');
#endif
	    }
	    AddQueue(mode + 0x0A);
	    return;
	}

    default:
    /* --------------Handle CTRL-ALT-DEL-------------- */

	if ((code == 0x53) && (ModeState & CTRL) && (ModeState & ALT))
	    ctrl_alt_del();

    /*
     *      Pick the right keymap
     */
	mode = ((ModeState & ~(NUM | ALT_GR)) >> 1) | (ModeState & 0x01);
	mode = state_code[mode];
	if(!mode && (ModeState & ALT_GR))
	    mode = 3;

	if((ModeState & CTRL && code < 14 && !(ModeState & ALT))
		|| (code < 70 && ModeState & NUM))
	    mode = 1;

	keyp_E0 = (char *)(*(scan_tabs[mode] + code));	/*[ keyp ]*/
    /*
     *      Apply special modifiers
     */
	if (ModeState & ALT && !(ModeState & CTRL))	/* Changed to support CTRL-ALT */
	    keyp_E0 = (char *)(((int) keyp_E0) | 0x80); /* META-.. */	/*[ keyp ]*/
	if (!keyp_E0)			/* non meta-@ is 64 */	/*[ keyp ]*/
	    keyp_E0 = (char *) '@';	/*[ keyp ]*/
	if (ModeState & CTRL && !(ModeState & ALT))	/* Changed to support CTRL-ALT */
	    keyp_E0 = (char *)(((int) keyp_E0) & 0x1F); /* CTRL-.. */	/*[ keyp ]*/
	if (((int)keyp_E0) == '\r')	/*[ keyp ]*/
	    keyp_E0 = (char *) '\n';	/*[ keyp ]*/
	AddQueue((unsigned char) keyp_E0);
    }
}

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress(void)
{
    set_irq();
    return chq_getch(&ttys[0].inq, 1);
}

#endif
