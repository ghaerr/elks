/**************************************
 * Direct video memory display driver *
 * Saku Airila 1996                   *
 **************************************
 
 * Modified for the Psion Series 3
 * Simon Wood. 12th June 1999
 */
 

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#ifdef CONFIG_CONSOLE_DIRECT

/* public interface of console.c: */

/* void con_charout (char Ch); */
/* void Console_set_vc (int N); */
/* int Console_write (struct tty * tty); */
/* void Console_release (struct inode * inode, struct file * file); */
/* int Console_open (struct inode * inode, struct file * file); */
/* struct tty_ops dircon_ops; */
/* void init_console(void); */
 
#include "console.h"
 
#define CONSOLE_NAME "console"
#define WIDTH 60
#define HEIGHT 12
#define MAX_CONS 3

char power_state = 1;

typedef
struct ConsoleTag
{
   int xpos, ypos;
#ifdef CONFIG_VIRTUAL_CONSOLE
   char screen [ HEIGHT ][ WIDTH ];
#endif
} Console;

/* allocate stores for virtual consoles */
#ifdef CONFIG_VIRTUAL_CONSOLE
static Console Con[ MAX_CONS ];
static Console * Visible;
#else
static Console Con[ 1 ];
static Console * Visible = &Con[0];
#endif

/* from keyboard.c */
extern int Current_VCminor;

/* prototype for LCD functions in con_asm.s */
void LCD_ScrollUp();
void LCD_Position();
void LCD_ClearLine();
void LCD_WriteChar();

extern int AddQueue(); /* From xt_key.c */
extern int GetQueue();

void WriteChar( C, Ch )
register Console * C;
char Ch;
{
	int loopx, loopy;

	if (Ch == '\f')
	{
		C->xpos = 0;
		C->ypos = 0;
	
		if ( Visible == &Con[ Current_VCminor ])
			LCD_Position(C->xpos, C->ypos);

		for( loopy = 0; loopy < HEIGHT; loopy ++)
		{
#ifdef CONFIG_VIRTUAL_CONSOLE
			for( loopx = 0; loopx < WIDTH; loopx++)
				C->screen[ loopy ][ loopx ] = ' ';
#endif
			if ( Visible == &Con[ Current_VCminor ])
				LCD_ClearLine( loopy );
		}
		return;
	}
			

	if (Ch == '\b')
	{
		if (C->xpos > 0)
		{
			C->xpos --;

			if ( Visible == &Con[ Current_VCminor ])
				LCD_Position(C->xpos, C->ypos);
		}
		return;
	}

	if (Ch == '\r')
	{
		C->xpos = 0;

		if ( Visible == &Con[ Current_VCminor ])
			LCD_Position(C->xpos, C->ypos);
		
		return;
	}	

	if (Ch == '\n') 
	{
		C->ypos ++;

		if ( Visible == &Con[ Current_VCminor ])
			LCD_Position(C->xpos, C->ypos);

		if (C->ypos == HEIGHT)
		{
			C->ypos --;
#ifdef CONFIG_VIRTUAL_CONSOLE
			for (loopy = 0; loopy < HEIGHT - 1 ; loopy ++)
				for (loopx = 0 ; loopx < WIDTH ; loopx ++)
					C->screen[ loopy ][ loopx ] = C->screen[ loopy + 1][ loopx ];

			for (loopx = 0 ; loopx < WIDTH ; loopx ++)
				C->screen[ C->ypos ][ loopx ] = ' ';
#endif
			if ( Visible == &Con[ Current_VCminor ])
			{
				LCD_ScrollUp();
				LCD_ClearLine( C->ypos );
				LCD_Position( C->xpos, C->ypos );
			}
		}
		return;
	}

	if ( Visible == &Con[ Current_VCminor ])
	      	LCD_WriteChar( Ch);

#ifdef CONFIG_VIRTUAL_CONSOLE
   	C->screen[ C->ypos ][ C->xpos ] = Ch;
#endif
	
	C->xpos ++;

	if (C->xpos == WIDTH)
		{
		WriteChar( C, '\r' );
		WriteChar( C, '\n' );
		}
}

void con_charout( Ch )
char Ch;
{
   WriteChar( Visible, Ch );
}

void LCD_Refresh()
{
#ifdef CONFIG_VIRTUAL_CONSOLE
	int loopx, loopy;

	for ( loopy = 0; loopy < HEIGHT; loopy++ )
	{
		LCD_Position( 0, loopy);

		for ( loopx = 0; loopx < WIDTH; loopx++ )
			LCD_WriteChar( Visible->screen[ loopy ][ loopx ]);
	}

	LCD_Position( Visible->xpos, Visible->ypos);
#else
	WriteChar(Visible, '\f');
#endif
	return;
}
		
			
/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X. 
 */
void Console_set_vc( N )
int N;
{
#ifdef CONFIG_VIRTUAL_CONSOLE
	/* Select Virtual console and display it */
	if( N < 0 || N >= MAX_CONS )
    		return;

	if( Visible == &Con[ N ] )
    		return;
    
  	Visible = &Con[ N ];
  	Current_VCminor = N;

	LCD_Refresh();
#endif	/* CONFIG_VIRTUAL_CONSOLE */
}

int Console_write(tty)
register struct tty * tty;
{
   int cnt = 0;
   unsigned char ch;
#ifdef CONFIG_VIRTUAL_CONSOLE
   Console * C = &Con[tty->minor];
#else 
   Console * C = &Con[0];	/* use default console: This is probably wrong */
#endif	/* CONFIG_VIRTUAL_CONSOLE */
   
   while (tty->outq.len != 0) { 
	chq_getch(&tty->outq, &ch, 0);
	WriteChar( C, ch);
	cnt++;
   };
   
   return cnt;
}

void Console_release( inode, file )
struct inode *inode;
struct file *file;
{
    /* Do nothing */
}

int Console_open( inode, file )
struct inode *inode;
struct file *file;
{
   int minor = MINOR(inode->i_rdev);
   
#ifdef CONFIG_VIRTUAL_CONSOLE
   if( minor >= MAX_CONS )
#else
   if( minor !=0 )
#endif 
      return -ENODEV;

   return 0;
}

struct tty_ops dircon_ops = {
	Console_open,
	Console_release, /* Do not remove this, or it crashes */
	Console_write,
	NULL,
	NULL,
};

void power_off()
{
	power_state = 0;

	printk("LCD powered off\n");
}

void power_on()
{
	power_state = 1;

	printk("LCD powered on\n");
}

void init_console()
{
   	unsigned int i;
	register Console * temp;

#ifdef CONFIG_VIRTUAL_CONSOLE
   	for( i = 1; i < MAX_CONS; i++ )
   	{
		temp = &Con[ i ];
		temp->xpos = 0;
		temp->ypos = 0;
		WriteChar( temp, '\f');
		
		WriteChar( temp, '0'+ i);
   	}
#endif	/* CONFIG_VIRTUAL_CONSOLE */

	Visible = &Con[ 0 ];

	Visible->xpos = 0;
	Visible->ypos = 0;
	WriteChar( Visible, '\f');
   

#ifdef CONFIG_VIRTUAL_CONSOLE   
   	printk("Console: Direct dumb (%d virtual consoles)\n", MAX_CONS);
#else
   	printk("Console: Direct dumb (no screen store)\n");
#endif
}

#endif	/* CONFIG_CONSOLE_DIRECT */

