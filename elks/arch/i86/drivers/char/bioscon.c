/*
 *	Moronic BIOS console
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

#ifdef CONFIG_CONSOLE_BIOS 

void con_charout(c)
char c;
{
	char tmp=c;
	/* Page 0 attribute 7 write character */

#asm
	mov bx,#0x7
	mov ah,#0xe
	int 0x10
#endasm
}

#if 0
int bioscon_lseek(inode,file,offset,origin)
struct inode *inode;
struct file *file;
off_t offset;
int origin;
{
	return -ESPIPE;
}
#endif

int bioscon_write(tty)
struct tty * tty;
{
        int cnt = 0;
        unsigned char ch;

        while (tty->outq.len != 0) {
                chq_getch(&tty->outq, &ch, 0);
                con_charout(ch);
                cnt++;
        };
        return cnt;
}

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
#if 0
	printk("BIOSCON: open %d\n", minor);
#endif
	if(minor!=0)
		return -ENODEV;
	return 0;
}

struct tty_ops bioscon_ops = {
	bioscon_open,
	bioscon_release, /* Do not remove this, or it crashes */
	bioscon_write,
	NULL,
	NULL,
};

void init_console()
{
	printk("Console: BIOS(%dx%d)\n",
		setupb(7),setupb(14));
}

#endif
