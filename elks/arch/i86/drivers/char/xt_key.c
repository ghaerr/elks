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
#include <arch/system.h>

#ifdef CONFIG_CONSOLE_DIRECT

extern struct tty ttys[];
extern void AddQueue(unsigned char Key);
void set_leds(void);

#define ESC	 27	/* ascii value for Escape*/
#define DEL_SCAN 0x53	/* scan code for Delete key*/

/*
 * Include the relevant keymap.
 */
#include "KeyMaps/keymaps.h"

int Current_VCminor = 0;
int kraw = 0;
static unsigned int ModeState = 0;	/* also led state*/

/*
 *	Keyboard state - the poor little keyboard controller hasnt
 *	got the brains to remember itself.
 *
 *********************************************
 * Changed this a lot. Made it work, too. ;) *
 * SA 1996                                   *
 *********************************************/

#define LSHIFT	0x01
#define RSHIFT	0x02
#define CTRL	0x04
#define ALT	0x08
#define CAPS	0x10
#define NUM	0x20
#define ALT_GR	0x40
#define SSC	0xC0	/* simple scan code*/

static unsigned char tb_state[] = {
    SSC, CTRL, SSC, SSC,			/*1C->1F*/
    SSC, SSC, SSC, SSC, SSC, SSC, SSC, SSC,	/*20->27*/
    SSC, SSC, LSHIFT, SSC, SSC, SSC, SSC, SSC,	/*28->2F*/
    SSC, SSC, SSC, SSC, SSC, SSC, RSHIFT, SSC,	/*30->37*/
    ALT, SSC, CAPS,				/*38->3A*/
    'a', 'b', 'c', 'd', 'e',			/*3B->3F, Function Keys*/
    'f', 'g', 'h', 'i', 'j',			/*40->44, Function Keys*/
    NUM, SSC, SSC,				/*45->47*/
    SSC, SSC, SSC, SSC, SSC, SSC, SSC, SSC,	/*48->4F*/
    SSC, SSC, SSC, SSC, SSC, SSC, SSC, 'k', 'l' /*50->58, F11-F12*/
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

/*************************************************************************
 * Queue for input received but not yet read by the application. SA 1996 *
 * There needs to be many buffers if we implement virtual consoles...... *
 *************************************************************************/

void xtk_init(void)
{
    set_leds();		/* turn off numlock led*/

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
    static int E0Prefix = 0;
    static int capslocktoggle =0;
    int code, mode;
    int E0key = 0;
    unsigned int key;
    int keyReleased;

    code = inb_p((void *) KBD_IO);

    /* Necessary for the XT. */
    mode = inb_p((void *) KBD_CTL);
    //printk("\nkey read1:%X,mode:%X,ModeState:%d\n",code,mode,ModeState);    
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
    if (E0Prefix) {
	E0key = 1;
	E0Prefix = 0;
    }

    /* high bit set when key released */
    keyReleased = code & 0x80;
    code &= 0x7F;

    //printk("scan %x\n", code);
    /*
     * Classify scancode such that
     *  mode = 00xx xxxxB, Status key
     *         01xx xxxxB, Function key
     *         10xx xxxxB, Control key
     *         11xx xxxxB, Simple Scan Code
     */
    if (code >= 0x1C)
	mode = tb_state[code - 0x1C]; /*is key a modifier key?*/
    else
        mode = SSC;

    if (!(mode & 0xC0)) {
    /* --------------Process status keys-------------- */
#if defined(CONFIG_KEYMAP_DE) || defined(CONFIG_KEYMAP_SE)
			   /* || defined(CONFIG_KEYMAP_ES) */
	/* ALT_GR has a E0 prefix*/
	if ((mode == ALT) && E0key)
	    mode = ALT_GR;
#endif
	if (keyReleased) {
	    if (mode == CAPS)
		capslocktoggle = !capslocktoggle;
	    if (mode != CAPS || !capslocktoggle)
		ModeState &= ~mode;
	} else
	    ModeState |= mode;
	set_leds();
	return;
    }
    
    /* no further processing on key release for non-status keys*/
    if (keyReleased)
	return;

    switch(mode & 0xC0) {
    /* --------------Handle Function keys-------------- */
    case 0x40:	/* F1 .. F10*/
    /* F11 and F12 function keys need 89 byte table like keys-de.h */
    /* function keys are not posix standard here */
    
	/* AltF1-F3 are console switch*/
	if ((ModeState & ALT) || code <= 0x3D) {/* temp console switch on F1-F3*/
	    Console_set_vc(code - 0x3B);
	    return;
	}

	AddQueue(ESC);		/* F1 = ESC a, F2 = ESC b, etc*/
	AddQueue(mode);
	return;

    /* --------------Handle extended scancodes-------------- */
    case 0x80:
	if (E0key) {		/* Is extended scancode? */
	    mode &= 0x3F;
	    if (mode) {
		AddQueue(ESC);	/* Up=0x37 -> ESC [ A, Down=0x38 -> ESC [ B, etc*/
#ifdef CONFIG_EMUL_ANSI
		AddQueue('[');
#endif
	    }
	    //printk("key 0%o 0x%x\n", mode, mode);
	    AddQueue(mode + 10);
	    return;
	}
	/* fall through*/

    default:
    /* --------------Handle simple scan codes-------------- */
	if (code == DEL_SCAN && ((ModeState & (CTRL|ALT)) == (CTRL|ALT)))
	    ctrl_alt_del();

        /* Pick the right keymap determined by ModeState*/
	mode = ((ModeState & ~(NUM | ALT_GR)) >> 1) | (ModeState & 0x01);
	mode = state_code[mode];
	
	if (!mode && (ModeState & ALT_GR))
	    mode = 3; /*ctrl_alt table*/

	if ((((ModeState & (CTRL|ALT)) == CTRL) && code < 14) ||
	     ((ModeState & NUM) && code < 70))
	    mode = 1; /*shift keys*/

	/* now read the key code from the selected table by mode */
	key = *(scan_tabs[mode] + code);

        /* Apply special modifiers*/
	if ((ModeState & (CTRL|ALT)) == ALT)
	    key |= 0x80;	/* META-.. */
	    
	if (!key)		/* non meta-@ is 64 */
	    key = '@';
	    
	if ((ModeState & (CTRL|ALT)) == CTRL)
	    key &= 0x1F;	/* CTRL-.. */
	    
#ifdef CONFIG_EMUL_ANSI
	code = mode = 0;
	switch (key) {
	case 0270: code = 'A'; break;			/* up*/
	case 0262: code = 'B'; break;			/* down*/
	case 0266: code = 'C'; break;			/* right*/
	case 0264: code = 'D'; break;			/* left*/
	case 0267: code = 'H'; break;			/* home*/
	case 0261: code = 'F'; break;			/* end*/
	case 0272: code = '2'; mode = '~'; break;	/* insert*/
	case 0271: code = '5'; mode = '~'; break;	/* page up*/
	case 0263: code = '6'; mode = '~'; break;	/* page dn*/
	//default: if (key > 127) printk("Unknown key (0%o 0x%x)\n", key, key);
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
	//printk("key 0%o 0x%x\n", key, key);
	AddQueue(key);
    }
}

/* LED routines from MINIX 2*/

/* Standard and AT keyboard.  (PS/2 MCA implies AT throughout.) */
#define KEYBD		0x60	/* I/O port for keyboard data */

/* AT keyboard. */
#define KB_COMMAND	0x64	/* I/O port for commands on AT */
#define KB_STATUS	0x64	/* I/O port for status on AT */
#define KB_ACK		0xFA	/* keyboard ack response */
#define KB_OUT_FULL	0x01	/* status bit set when keypress char pending */
#define KB_IN_FULL	0x02	/* status bit set when not ready to receive */
#define LED_CODE	0xED	/* command to keyboard to set LEDs */
#define MAX_KB_ACK_RETRIES 0x1000	/* max #times to wait for kb ack */
#define MAX_KB_BUSY_RETRIES 0x1000	/* max #times to loop while kb busy */
#define KBIT		0x80	/* bit used to ack characters to keyboard */

/* Wait until the controller is ready; return zero if this times out. */
static int kb_wait(void)
{
    int retries;
    unsigned char status;

    retries = MAX_KB_BUSY_RETRIES + 1;	/* wait until not busy */
    while (--retries != 0 && (status = inb_p(KB_STATUS)) & (KB_IN_FULL|KB_OUT_FULL)) {
	if (status & KB_OUT_FULL)
	    inb_p(KEYBD);		/* discard */
    }
    return(retries);		/* nonzero if ready */
}

/* Wait until kbd acknowledges last command; return zero if this times out. */
static int kb_ack(void)
{
    int retries;

    retries = MAX_KB_ACK_RETRIES + 1;
    while (--retries != 0 && inb_p(KEYBD) != KB_ACK)
	;			/* wait for ack */
    return(retries);		/* nonzero if ack received */
}

/* Set the LEDs on the caps, num, and scroll lock keys */
void set_leds(void)
{
    if (arch_cpu <= 5) return;	/* PC/XT doesn't have LEDs */

printk("1");
    kb_wait();			/* wait for buffer empty  */
    outb_p(LED_CODE, KEYBD);	/* prepare keyboard to accept LED values */
printk("2");
    kb_ack();			/* wait for ack response  */
printk("3");

    kb_wait();			/* wait for buffer empty  */
printk("4");
    outb_p(ModeState, KEYBD);	/* give keyboard LED values */
printk("5");
    kb_ack();			/* wait for ack response  */
printk("[%x]", ModeState);
}

#endif /* CONFIG_CONSOLE_DIRECT*/
