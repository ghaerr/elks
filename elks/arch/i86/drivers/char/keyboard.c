
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

#include "console.h"	/* for number of VC's */

extern struct tty ttys[];

#define ESC 27

#define LSHIFT 	0x0100
#define RSHIFT 	0x0400
#define CTRL 	0x0200
#define ALT 	0x0800	/* Diamond key */
#define MENU	0x1000
#define PSION   0x8000

#define ANYSHIFT LSHIFT|RSHIFT

int AddQueue();
int GetQueue();
int KeyboardInit();

int Current_VCminor = 0;

/* from console.c */
extern char power_state;

int KeyboardInit()
{
}

/*
 *  Psion keyboard is very different to PC, this is a quick stab to get
 *  something going. Simon Wood 12th June 1999
 */
 
void keyboard_irq(/*irq,regsi*/)
/*int irq;
struct pt_regs *regs;*/
{
	int modifiers;
	int key;
	static int okey = 0;

   	modifiers = psiongetchar();
   	key = (modifiers & 0x00FF);

	/* FIXME: Need to add some form of Key repeat.. */

	if( modifiers & PSION)
	{
		if ( key == 0x31 && power_state == 1 ) /* Psion + 1 */
			power_off();

		if ( key == 0x20 && power_state == 0 ) /* Psion + Space */
			power_on();
	
		return;
	}

	if (power_state == 0 ) return;		/* prevent key presses when off */
      
   	if( modifiers & MENU )
   	{
      		if( key < 0x40 && key > 0x30 ) /* MENU + 1..9 */
      		{
#ifdef CONFIG_VIRTUAL_CONSOLE
        		Console_set_vc( key - 0x31 );
        		return;
#else
        		AddQueue( ESC );
        		AddQueue( key - 0x31 + 'a' );
        		return;
#endif
      		}
   	}
	if (!okey || key) {
		if (key) {
			okey = key;
		}
		return;
	}
	key = okey;
	okey = 0;

   	switch( key )
   	{
      		case 0x18 :  /* Arrow up */
         		AddQueue( ESC );
         		AddQueue( 'A' );
         		return;
      		case 0x19 :  /* Arrow down */
         		AddQueue( ESC );
         		AddQueue( 'B' );
         		return;
      		case 0x1A :  /* Arrow right */
         		AddQueue( ESC );
         		AddQueue( 'C' );
         		return;
      		case 0x1B :  /* Arrow left */
         		AddQueue( ESC );
         		AddQueue( 'D' );
         		return;
		case '\r' :
			AddQueue( '\n' );
			return;
      		default :
         		AddQueue( key );
         		return;
      	}
}

/* Ack.  We can't add a character until the queue's ready */

int AddQueue( Key )
unsigned char Key;
{
   register struct tty * ttyp = &ttys[Current_VCminor];
   if (ttyp->inq.size != 0) {
	   chq_addch(&ttyp->inq, Key);
   }
   return 0;
}
#if 0
int GetQueue()
{
   return chq_getch(&ttys[0].inq, 0, 0);
}
#endif

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress()
{
	return chq_getch(&ttys[0].inq, 0, 1);
}

#endif /* CONFIG_CONSOLE_DIRECT */
