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
#include <linuxmt/kernel.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#ifdef CONFIG_CONSOLE_DIRECT

#if 0
/* public interface of console.c: */

void con_charout(char Ch);
void Console_set_vc(int N);
int Console_write(struct tty *tty);
void Console_release(struct inode *inode, struct file *file);
int Console_open(struct inode *inode, struct file *file);
struct tty_ops dircon_ops;
void init_console(void);
#endif

#include "console.h"

#define CONSOLE_NAME "console"
#define WIDTH 60
#define HEIGHT 12

char power_state = 1;

typedef struct ConsoleTag {
    int xpos, ypos;
#ifdef CONFIG_VIRTUAL_CONSOLE
    char screen[HEIGHT][WIDTH];
#endif
} Console;

/* allocate stores for virtual consoles */
#ifdef CONFIG_VIRTUAL_CONSOLE
static Console Con[MAX_CONSOLES];
static Console *Visible;
#else
static Console Con[1];
static Console *Visible = &Con[0];
#endif

/* from keyboard.c */
extern int Current_VCminor;

void LCD_ScrollUp();		/* prototype for LCD functions in con_asm.s */
void LCD_Position();
void LCD_ClearLine();
void LCD_WriteChar();

extern void AddQueue(unsigned char Key);	/* From xt_key.c */

void WriteChar(register Console * C, char c)
{
#ifdef CONFIG_VIRTUAL_CONSOLE
    int loopx
#endif
    int loopy;

    switch (c) {
    case '\f':
	C->xpos = C->ypos = 0;

	if (Visible == &Con[Current_VCminor])
	    LCD_Position(C->xpos, C->ypos);

	for (loopy = 0; loopy < HEIGHT; loopy++) {

#ifdef CONFIG_VIRTUAL_CONSOLE

	    for (loopx = 0; loopx < WIDTH; loopx++)
		C->screen[loopy][loopx] = ' ';

#endif

	    if (Visible == &Con[Current_VCminor])
		LCD_ClearLine(loopy);
	}
	return;

    case '\b':
	if (C->xpos > 0) {
	    C->xpos--;

	    if (Visible == &Con[Current_VCminor])
		LCD_Position(C->xpos, C->ypos);
	}
	return;

    case '\r':
	C->xpos = 0;

	if (Visible == &Con[Current_VCminor])
	    LCD_Position(C->xpos, C->ypos);

	return;

    case '\n':
	C->ypos++;

	if (Visible == &Con[Current_VCminor])
	    LCD_Position(C->xpos, C->ypos);

	if (C->ypos == HEIGHT) {
	    C->ypos--;

#ifdef CONFIG_VIRTUAL_CONSOLE

	    for (loopy = 0; loopy < HEIGHT - 1; loopy++)
		for (loopx = 0; loopx < WIDTH; loopx++)
		    C->screen[loopy][loopx] = C->screen[loopy + 1][loopx];

	    for (loopx = 0; loopx < WIDTH; loopx++)
		C->screen[C->ypos][loopx] = ' ';

#endif

	    if (Visible == &Con[Current_VCminor]) {
		LCD_ScrollUp();
		LCD_ClearLine(C->ypos);
		LCD_Position(C->xpos, C->ypos);
	    }
	}
	return;

    default:
	break;
    }

    if (Visible == &Con[Current_VCminor])
	LCD_WriteChar(c);

#ifdef CONFIG_VIRTUAL_CONSOLE

    C->screen[C->ypos][C->xpos] = c;

#endif

    C->xpos++;

    if (C->xpos == WIDTH) {
	WriteChar(C, '\r');
	WriteChar(C, '\n');
    }
}

void con_charout(char Ch)
{
    if (Ch == '\n')
	WriteChar(Visible, '\r');
    WriteChar(Visible, Ch);
}

void LCD_Refresh(void)
{

#ifdef CONFIG_VIRTUAL_CONSOLE

    int loopx, loopy;

    for (loopy = 0; loopy < HEIGHT; loopy++) {
	LCD_Position(0, loopy);

	for (loopx = 0; loopx < WIDTH; loopx++)
	    LCD_WriteChar(Visible->screen[loopy][loopx]);
    }

    LCD_Position(Visible->xpos, Visible->ypos);

#else

    WriteChar(Visible, '\f');

#endif
}

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X. 
 */
void Console_set_vc(int N)
{
#ifdef CONFIG_VIRTUAL_CONSOLE

    /* Select Virtual console and display it */
    if (N < 0 || N >= MAX_CONSOLES)
	return;

    if (Visible == &Con[N])
	return;

    Visible = &Con[N];
    Current_VCminor = N;

    LCD_Refresh();

#endif
}

int Console_write(register struct tty *tty)
{
    Console *C;
    int cnt = 0;

#ifdef CONFIG_VIRTUAL_CONSOLE
    C = &Con[tty->minor];
#else
    C = &Con[0];		/* use default console: This is probably wrong */
#endif

    while (tty->outq.len > 0) {
	WriteChar(C, (char)tty_outproc(tty));
	cnt++;
    }

    return cnt;
}

void Console_release(struct inode *inode, struct file *file)
{
    /* Do nothing */
}

int Console_open(struct inode *inode, struct file *file)
{
    unsigned short int minor = MINOR(inode->i_rdev);

#ifdef CONFIG_VIRTUAL_CONSOLE

    if (minor >= MAX_CONSOLES)
#else

    if (minor)
#endif
	return -ENODEV;

    return 0;
}

/*@-type@*/

struct tty_ops dircon_ops = {
    Console_open,
    Console_release,		/* Do not remove this, or it crashes */
    Console_write,
    NULL,
    NULL,
};

/*@+type@*/

void power_off(void)
{
    power_state = 0;

    printk("LCD powered off\n");
}

void power_on(void)
{
    power_state = 1;

    printk("LCD powered on\n");
}

void init_console(void)
{
#ifdef CONFIG_VIRTUAL_CONSOLE

    unsigned int i;
    register Console *temp;

    for (i = 1; i < MAX_CONSOLES; i++) {
	temp = &Con[i];
	temp->xpos = 0;
	temp->ypos = 0;
	WriteChar(temp, '\f');

	WriteChar(temp, '0' + i);
    }

#endif

    Visible = &Con[0];

    Visible->xpos = 0;
    Visible->ypos = 0;
    WriteChar(Visible, '\f');

#ifdef CONFIG_VIRTUAL_CONSOLE

    printk("Console: Direct dumb (%u virtual consoles)\n", MAX_CONSOLES);

#else

    printk("Console: Direct dumb (no screen store)\n");

#endif
}

#endif
