/*
 *	Moronic BIOS console
 *	
 *	11th Nov 98	Re-wrote to work with ntty code
 *			Al - (ajr@ecs.soton.ac.uk)
 */

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/ntty.h>

#ifdef CONFIG_CONSOLE_BIOS 

#ifndef printd_tty /* This should go in debug.h */
#define printd_tty(_a,_b)
#endif

int read_kbd();

#ifdef CONFIG_BIOS_VT52
void CommandChar(c)
char c;
{
	switch(c) {
	  case 'H':
#asm
	    xor bh,bh
	    mov dx,#0x0
	    int 0x10
#endasm
	    break;
	  case 'A':
#asm
	    xor bh,bh
	    mov ah,#0x3
	    int 0x10
	    test ax,ax
	    je notzero1
	    dec DH
	    mov ah,#0x2
	    int 0x10
notzero1:
#endasm
	    break;
	}
}
#endif /* CONFIG_BIOS_VT52 */

void con_charout(c)
char c;
{
	char tmp=c;
	static InCmd;
#ifdef CONFIG_BIOS_VT52
	if(InCmd) {
	  CommandChar(c);
	}
	if(c==27) {
	  InCmd=1;
	  return;
	}
#endif /* CONFIG_BIOS_VT52 */

	/* \n\r handling */
	if (c == '\n') con_charout('\r');
	rawout(c);
}

/* This is an ntty compliant tty->write function */

int bioscon_write(tty)
register struct tty * tty;
{
	int cnt = 0, chi;
	unsigned char ch;
	while (tty->outq.len != 0) {
		chq_getch(&tty->outq, &ch, 0);
		con_charout(ch);
		cnt++;
	}
	return cnt;
}

/* To date this is the only ntty->read function, so I am making up the spec */

void bioscon_read(tty)
register struct tty * tty;
{
	char c = read_kbd();

	if (c == '\r') {
		c = '\n';
	}
	chq_addch(&tty->inq, c);
}


/* Hell, why not save ourselves .0001s in compile time */

#asm
export _wait_for_keypress
_wait_for_keypress:
	call _read_kbd
	ret

export _read_kbd
_read_kbd:
	xor ah,ah
	int 0x16
	xor ah,ah
	ret

export _rawout
_rawout:
	push bp
	mov bp,sp
	mov al,4[bp]
	mov bx,#0x7
	mov ah,#0xe
	int 0x10
	pop bp
	ret
#endasm


void bioscon_release(inode,file)
struct inode *inode;
struct file *file;
{
	return;
}

int bioscon_open(inode,file)
struct inode *inode;
struct file *file;
{
	int minor = MINOR(inode->i_rdev);
	printd_tty("BIOSCON: open %d\n", minor);
	if(minor!=0)
		return -ENODEV;
	return 0;
}

struct tty_ops bioscon_ops=
{
	bioscon_open,		/* open */
	bioscon_release,	/* release */
	bioscon_write,		/* write */
	bioscon_read,		/* read */
	NULL,
};

void init_console()
{
	printk("Console: BIOS(%dx%d)\n",
		setupb(7),setupb(14));
}


#if 0

/* Everything below this point was written as if the console driver *
 * was a character device driver rather than a ntty driver.         *
 * Makes absolutly no sense so I replaced it all.                   *
 * Al (ajr@ecs.soton.ac.uk) - 11 Nov 98                             */

int bioscon_lseek(inode,file,offset,origin)
struct inode *inode;
struct file *file;
off_t offset;
int origin;
{
	return -ESPIPE;
}

int bioscon_write(inode,file,buf,count)
struct inode *inode;
struct file *file;
char *buf;
int count;
{
	int len=0;
	for (; len < count; len++)	
	{
		con_charout(peekb(current->t_regs.ds, buf++));
	}
	return len;
}

int bioscon_read(inode,file,buf,count) /* FIXME - URGENT */
struct inode *inode;
struct file *file;
char *buf;
int count;
{
	int len=0;
	char c;
	printk("bioscon_read(count=%d)\n",count);
	for(; len < count; len++)
	{
		printk("Reading kbd\n");
		c=read_kbd();
		printk("Reading kbd\n");
		pokeb(current->t_regs.ds,buf++,c);
		rawout(c);
		if(c=='\n') break;
	}
	printk("Returning\n");
	return len;
}

/* This doesn't make sense! This should not be a char device, it should be *
 * a tty device  - Al 11th Nov 98 */
static struct file_operations bioscon_ops=
{
	bioscon_lseek,
	bioscon_read,
	bioscon_write,
	NULL,
	NULL,			/* Write select later */
	NULL,			/* Write ioctl later */
	bioscon_open,		/* Open the bios console */
	bioscon_release,	/* Close the bios console */
#ifdef BLOAT_FS
	NULL,			/* Fsync */
	NULL,			/* No media */
	NULL			/* No media */
#endif
};

void init_console()
{
	register_chrdev(TTY_MAJOR, "bioscon", &bioscon_ops);
	printk("Console: BIOS(%dx%d)\n",
		setupb(7),setupb(14));
}

#endif /* 0 */
#endif /* CONFIG_CONSOLE_BIOS */
