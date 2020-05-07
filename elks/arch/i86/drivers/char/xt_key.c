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
 ***************************************************************
 * Changed code to support int'l keys 42 + 45, enable caps lock*
 * preliminary F11+F12 support,                                *
 * add comments to increase readability                        *
 * Georg Potthast 2017                                         *
 * ************************************************************/

#include <linuxmt/config.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/signal.h>
#include <linuxmt/ntty.h>

#include <arch/io.h>
#include <arch/ports.h>
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

#define SSC 0xC0 /*Simple Scan Code*/

static unsigned char tb_state[] = {
    SSC, CTRL, SSC, SSC,			/*1C->1F*/
    SSC, SSC, SSC, SSC, SSC, SSC, SSC, SSC,	/*20->27*/
    SSC, SSC, LSHIFT, SSC, SSC, SSC, SSC, SSC,	/*28->2F*/
    SSC, SSC, SSC, SSC, SSC, SSC, RSHIFT, SSC,	/*30->37*/
    ALT, SSC, CAPS,				/*38->3A*/
    'a', 'b', 'c', 'd', 'e',			/*3B->3F, Function Keys*/
    'f', 'g', 'h', 'i', 'j',			/*40->44, Function Keys*/
    NUM, SSC, SSC,				/*45->47*/
    0xB7, SSC, SSC, 0xBA, SSC, 0xB9, SSC, SSC,	/*48->4F*/
    0xB8, SSC, SSC, SSC, SSC, SSC, SSC, 'k', 'l'/*50->58 F11-F12*/
};

static unsigned char state_code[] = {
    0,	/*0= All status are 0 */
    1,	/*1= SHIFT */
    0,	/*2= CTRL */
    1,	/*3= SHIFT CTRL */
    0,	/*4= ALT */
    1,	/*5= SHIFT ALT */
    3,	/*6= CTRL ALT */
    1,	/*7= SHIFT CTRL ALT */
    2,	/*8= CAPS */ /* CAPS >>1 */
    0,	/*9= CAPS SHIFT */
    2,	/*10= CAPS CTRL */
    0,	/*11= CAPS SHIFT CTRL */
    2,	/*12= CAPS ALT */
    0,	/*13= CAPS SHIFT ALT */
    2,	/*14= CAPS CTRL ALT */
    3,	/*15= CAPS SHIFT CTRL ALT */
};

/* tables defined in KeyMaps/keys-xx.h files*/
static unsigned char *scan_tabs[] = {
    xtkb_scan,		/*mode = 0*/
    xtkb_scan_shifted,	/*mode = 1*/
    xtkb_scan_caps,	/*mode = 2*/
    xtkb_scan_ctrl_alt,	/*mode = 3*/
};

/* Ack.  We can't add a character until the queue's ready
 */

void AddQueue(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    if (!tty_intcheck(ttyp, Key))
	chq_addch(&ttyp->inq, Key);
}

/*************************************************************************
 * Queue for input received but not yet read by the application. SA 1996 *
 * There needs to be many buffers if we implement virtual consoles...... *
 *************************************************************************/

void xtk_init(void)
{
    /* Set off the initial keyboard interrupt handler */

    if (request_irq(KBD_IRQ, keyboard_irq, NULL))
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
    static int capslocktoggle =0;
    int code, mode;
    register char *keyp_E0 = (char *)0;	/*[char *keyp; int E0]*/
    register char *IsReleasep;

    code = inb_p((void *) KBD_IO);

    /* Necessary for the XT. */
    mode = inb_p((void *) KBD_CTL);
    //printk("\nkey read:%X,mode:%X,ModeState:%d\n",code,mode,ModeState);    
    outb_p((unsigned char) (mode | 0x80), (void *) KBD_CTL);
    outb_p((unsigned char) mode, (void *) KBD_CTL);

    if (kraw) {
	AddQueue((unsigned char) code);
	return;
    }

    /*extended keys are preceded by a E0 scancode*/
    if (code == 0xE0) {		/* Remember this has been received */
	E0Prefix = 1;
	return;
    }

    /* temporary store EOPrefix in keyp_E0 e.g. for ALT_GR */
    if (E0Prefix) {
	keyp_E0 = (char *)1;	/*[ E0 ]*/
	E0Prefix = 0;
    }

/* the highest bit is set when the key was released. 
 * Save pressed/relased state in IsReleasep. Bit set = released */
    IsReleasep = (char *)(code & 0x80);
    code &= 0x7F; /* clear bit indicating released key */

    /*
     * Classify scancode such that
     *  mode = 00xx xxxxB, Status key
     *         01xx xxxxB, Function key
     *         10xx xxxxB, Control key
     *         11xx xxxxB, Simple Scan Code
     */
    if (code >= 0x1C) {
      mode = tb_state[code - 0x1C]; /*is key a modifier key?*/
    } else {
      mode = SSC;
    }
      
    /* --------------Process status keys-------------- */
    if (!(mode & 0xC0)) { /*not a simple scan code*/

#if defined(CONFIG_KEYMAP_DE) || defined(CONFIG_KEYMAP_SE) /* || defined(CONFIG_KEYMAP_ES) */
	/* ALT_GR has a EO prefix this is stored in keyp_E0 above */
	if ((mode == ALT) && ((int)keyp_E0 != 0))	/*[ E0 ]*/
	    mode = ALT_GR;
#endif
	if (IsReleasep) { /* the key was released */
	  if (mode == 16) {
	    if (capslocktoggle==0) {
	      capslocktoggle=1;
	    } else {
	      capslocktoggle=0;
	    }
	  } //mode == 16
	  if ((mode != 16) | (capslocktoggle == 0)) ModeState &= ~mode; /*clear*/
	} else {
	  ModeState |= mode;  /*set*/
	}

    /* did set the ModeState according to the modifier key and return now */
	return;
    }
    
    if (IsReleasep) /* a simple scan code was released */
	return;

    switch(mode & 0xC0) {
      
    /* --------------Handle Function keys-------------- */
    case 0x40:	/* F1 .. F10 - adding ESC plus mode byte to queue */
    /* F11 and F12 function keys need 89 byte table like keys-de.h */
    /* function keys are not posix standard here */
    
	//if (ModeState & ALT) {	/* AltF1-F3 are console switch*/
	if (code <= 0x3D) {		/* F1-F3 are console switch*/
	    Console_set_vc((unsigned) (code - 0x3B));
	    return;
	}

	AddQueue(ESC);
	AddQueue((unsigned char)mode);
	return;

    /* --------------Handle extended scancodes-------------- */
    case 0x80:
	if ((int)keyp_E0) {		/* Is extended scancode? */	/*[ E0 ]*/
	    mode &= 0x3F;
	    if (mode) {
		AddQueue(ESC);
#ifdef CONFIG_EMUL_ANSI
		AddQueue('[');
#endif
	    }
	    //printk("key read2:%X,mode:%X,ModeState:%d,mode+0A:%X\n",code,mode,ModeState,(mode + 0x0A));    
	    AddQueue(mode + 0x0A);
	    return;
	}

    default:
    /* --------------Handle CTRL-ALT-DEL-------------- */
	if ((code == 0x53) && (ModeState & CTRL) && (ModeState & ALT))
	    ctrl_alt_del();

    /* --------------Handle simple scan codes-------------- */	
    /*
     *      Pick the right keymap determined by the ModeState
     */
	mode = ((ModeState & ~(NUM | ALT_GR)) >> 1) | (ModeState & 0x01);
	mode = state_code[mode];
	
	if (!mode && (ModeState & ALT_GR))
	    mode = 3; /*ctrl_alt table*/

	if ((ModeState & CTRL && code < 14 && !(ModeState & ALT))
		|| (code < 70 && ModeState & NUM))
	    mode = 1; /*shift keys*/

	/* now read the key code from the selected table by mode */
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
	    
#ifdef CONFIG_EMUL_ANSI
	code = mode = 0;
	switch ((unsigned int)keyp_E0) {
	case 0xb7: code = 'H'; break;			/* home*/
	case 0xb1: code = 'F'; break;			/* end*/
	case 0xb9: code = '5'; mode = '~'; break;	/* page up*/
	case 0xb3: code = '6'; mode = '~'; break;	/* page dn*/
	}
	if (code) {
	    AddQueue(ESC);
	    AddQueue('[');
	    AddQueue(code);
	    if (mode)
		AddQueue(mode);
	    return;
	}
#endif
	//printk("keyp_E0:%X\n",keyp_E0);    
	AddQueue((unsigned char) keyp_E0);
    }
}

#endif
