#include <linuxmt/types.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/serial_reg.h>
#include <linuxmt/ntty.h>
#include <linuxmt/debug.h>

extern struct tty ttys[];

struct serial_info
{
	unsigned short io;
	unsigned char irq;
	unsigned char flags;
	unsigned char lcr;
	unsigned char mcr;
	unsigned int baud_rate;
	struct tty * tty;	
#define SERF_TYPE	15
#define SERF_EXIST	16
#define ST_8250		0
#define ST_16450	1
#define ST_16550	2
#define ST_UNKNOWN	15
};

#define RS_MINOR_OFFSET 4

#define CONSOLE_PORT 0

/* all boxes should be able to do 9600 at least, afaik 8250 works fine up to 19200 */
#define DEFAULT_BAUD_RATE 9600
#define DEFAULT_LCR UART_LCR_WLEN8
#define DEFAULT_MCR UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2

#define MAX_RX_BUFFER_SIZE 16

static struct serial_info ports[4]=
{
	{ 0x3f8,4,0,DEFAULT_LCR, DEFAULT_MCR, DEFAULT_BAUD_RATE,0 },
	{ 0x2f8,3,0,DEFAULT_LCR, DEFAULT_MCR, DEFAULT_BAUD_RATE,0 },
	{ 0x3e8,5,0,DEFAULT_LCR, DEFAULT_MCR, DEFAULT_BAUD_RATE,0 },
	{ 0x2e8,2,0,DEFAULT_LCR, DEFAULT_MCR, DEFAULT_BAUD_RATE,0 },
};

static char irq_port[4] = {3,1,0,2};

static int rs_in_use[4];

int rs_open(inode,file);
void rs_release(inode,file);
int rs_write(tty);
static int rs_ioctl(tty, cmd, arg);

struct tty_ops rs_ops = {
	rs_open,
	rs_release,
	rs_write,
	NULL,
	rs_ioctl
};

#if 0 /* Function stubs - implement to use as serial console */
void init_console() {}

void con_charout() {}
#endif

static int set_serial_info(info, new_info)
struct serial_info * info;
struct serial_info * new_info;
{
	int err; 
	struct inode * inode;
	
	err = verified_memcpy_fromfs(info, new_info, sizeof(struct serial_info));

	if (err != 0) return err;

	/* shutdown serial port and restart UART with new settings */

	/* witty cheat :) - either we do this (and waste some DS) or duplicate
	almost whole rs_release and rs_open (and waste much more CS) */
	inode->i_rdev = info->tty->minor;

	rs_release(inode, NULL);
	err = rs_open(inode, NULL);

	return err;
}

static int get_serial_info(info, ret_info)
struct serial_info * info;
struct serial_info * ret_info;
{
	return verified_memcpy_tofs(ret_info, info, sizeof(struct serial_info));
}

int rs_open(inode,file)
struct inode *inode;
struct file *file;
{
	int count;
	int divisor;
	int rs_minor = MINOR(inode->i_rdev) - RS_MINOR_OFFSET;

	printd_rs("RS_OPEN called\n");

	if (!(ports[rs_minor].flags & SERF_EXIST))
		return -ENODEV;

	/* is port already in use ? */
	if (rs_in_use[rs_minor])
		return -EBUSY;

	/* no, mark it in use */
	rs_in_use[rs_minor] = 1;

	/* clear RX buffer */
	inb_p(ports[rs_minor].io + UART_LSR);
	count = MAX_RX_BUFFER_SIZE;
	do {
		inb_p(ports[rs_minor].io + UART_RX);
		count--;
	} while ((count > 0) && (inb_p(ports[rs_minor].io + UART_LSR) & UART_LSR_DR));

	inb_p(ports[rs_minor].io + UART_IIR);
	inb_p(ports[rs_minor].io + UART_MSR);

	/* set serial port parameters to match ports[rs_minor] */

	/* set baud rate divisor, first lower, then higher byte */
	divisor = 115200 / ports[rs_minor].baud_rate;

	outb_p(UART_LCR_WLEN8 | UART_LCR_DLAB, ports[rs_minor].io + UART_LCR);
	outb_p(divisor & 0xff, ports[rs_minor].io + UART_DLL);
	outb_p((divisor >> 8) & 0xff, ports[rs_minor].io + UART_DLM);

	/* set wordlength to 8 bits, no parity, 1 stop bit */
	outb_p(ports[rs_minor].lcr, ports[rs_minor].io + UART_LCR);

	/* enable reciever data interrupt; FIXME: update code to utilize full interrupt interface */
	outb_p(UART_IER_RDI, ports[rs_minor].io + UART_IER);

	outb_p(ports[rs_minor].mcr, ports[rs_minor].io + UART_MCR);

	/* clear Line/Modem Status, Intr ID and RX register */
	inb_p(ports[rs_minor].io + UART_LSR);
	inb_p(ports[rs_minor].io + UART_RX);
	inb_p(ports[rs_minor].io + UART_IIR);
	inb_p(ports[rs_minor].io + UART_MSR);

#if 0 
/* this is old code */
/*	outb_p(UART_LCR_WLEN8 | UART_LCR_DLAB, ports[rs_minor].io + UART_LCR);
	outb_p(6, ports[rs_minor].io + UART_DLL);
	outb_p(0, ports[rs_minor].io + UART_DLM); */

	outb_p(UART_LCR_WLEN8, ports[rs_minor].io + UART_LCR);
	outb_p(UART_IER_RDI, ports[rs_minor].io + UART_IER);
	outb_p(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2, ports[rs_minor].io + UART_MCR);
/*	inb_p(ports[rs_minor].io + UART_LSR);
	inb_p(ports[rs_minor].io + UART_RX);
	inb_p(ports[rs_minor].io + UART_IIR);
	inb_p(ports[rs_minor].io + UART_MSR); */
#endif
	return 0;
}

void rs_release(inode,file)
struct inode *inode;
struct file *file;
{
	int rs_minor = MINOR(inode->i_rdev) - RS_MINOR_OFFSET;

	printd_rs("RS_RELEASE called\n");
	rs_in_use[rs_minor] = 0;
	outb_p(0, ports[rs_minor].io + UART_IER);

}

int rs_write(tty)
struct tty * tty;
{
	char ch;
	int rs_minor = tty->minor - RS_MINOR_OFFSET;
	while (chq_getch(&tty->outq, &ch, 0) != -1) {
		while (!(inb_p(ports[rs_minor].io + UART_LSR) & UART_LSR_TEMT));
		outb(ch, ports[rs_minor].io + UART_TX);
	}
}

int rs_ioctl(tty, cmd, arg)
struct tty * tty;
int cmd;
char * arg;
{
	int retval;
	int sp = tty->minor - RS_MINOR_OFFSET;
	
	/* few sanity checks should be here */

	printk("rs_ioctl: sp = %d, cmd = %d\n", sp, cmd);

	switch (cmd)
	{
		/* Unlike Linux we use verified_memcpy*fs() which calls verify_area() for us */
		case TIOCSSERIAL: 
		retval = set_serial_info(&ports[sp], arg);

		case TIOCGSERIAL: 
		retval = get_serial_info(&ports[sp], arg);
	}

	return retval;
}

void receive_chars(sp)
register struct serial_info *sp;
{
	char ch;

	do {
		ch = inb_p(sp->io + UART_RX);
		if (ch == '\r')
			ch = '\n';
		chq_addch(&sp->tty->inq, ch);
	} while (inb_p(sp->io + UART_LSR) & UART_LSR_DR);
}
/*
void transmit_chars(sp)
struct serial_info *sp;
{
	char ch;

	if (chq_getch(&sp->tty->outq, &ch, 0) != -1)
		outb(ch, sp->io + UART_TX);
		
}
*/
int rs_irq(irq,regs)
int irq;
struct pt_regs *regs;
{
	int status;
	register struct serial_info * sp;
	unsigned char ch;
	printd_rs1("Serial interrupt %d recieved.\n", irq);

	sp = &ports[irq_port[irq -2]];
	do {
		status = inb_p(sp->io + UART_LSR);
		if (status & UART_LSR_DR) {
			receive_chars(sp);
		}
/*		if (status & UART_LSR_THRE)
			transmit_chars(sp); */
	} while (!(inb_p(sp->io + UART_IIR) & UART_IIR_NO_INT));
	
}

int rs_probe(sp)
register struct serial_info *sp;
{
	/* DS WASTING: is it just me or is this scratch2 redundant (srcatch can be reused) ? */
	int count;
	int scratch,scratch2;
	int status1,status2;
	
	scratch=inb(sp->io+UART_IER);
	outb_p(0,sp->io+UART_IER);
	scratch=inb_p(sp->io+UART_IER);
	outb_p(scratch,sp->io+UART_IER);
	if(scratch)
		return -1;

	/* this code is weird, IMO */
	scratch2=inb_p(sp->io+UART_LCR);
	outb_p(scratch2 | UART_LCR_DLAB, sp->io+UART_LCR);
	outb_p(0,sp->io+UART_EFR);
	outb_p(scratch2,sp->io+UART_LCR);

	outb_p(UART_FCR_ENABLE_FIFO, sp->io+UART_FCR);

	/* upper two bits of IIR define UART type, but according to both RB's intlist and HelpPC this code is wrong, see comments marked with [*] */
	scratch=inb_p(sp->io+UART_IIR)>>6;
	switch(scratch)
	{
		case 0:
			sp->flags=SERF_EXIST|ST_16450;
			break;
		case 1: /* [*] this denotes broken 16550 UART, not 16550A or any newer type */
			sp->flags=ST_UNKNOWN;
			break;
		case 2: /* invalid combination */
		case 3:	/* Could be a 16650.. we dont care */
			/* [*] 16550A or newer with enabled FIFO buffers */
			sp->flags=SERF_EXIST|ST_16550;
			break;
	}
	
	/* 8250 UART if scratch register isn't present */
	if(!scratch)
	{
		scratch=inb_p(sp->io+UART_SCR);
		outb_p(0xA5,sp->io+UART_SCR);
		status1=inb_p(sp->io+UART_SCR);
		outb_p(0x5A,sp->io+UART_SCR);
		status2=inb_p(sp->io+UART_SCR);
		outb_p(scratch,sp->io+UART_SCR);
		
		if((status1 != 0xA5) || (status2 != 0x5A))
			sp->flags=SERF_EXIST|ST_8250;
	}
	
	/*
	 *	Reset the chip
	 */
	 
	outb_p(0x00,sp->io+UART_MCR);
	/* clear RX and TX FIFOs */
	outb_p(UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT,sp->io+UART_FCR);
	/* clear RX register */
	count = MAX_RX_BUFFER_SIZE;
	do {
		inb_p(sp->io + UART_RX);
		count--;
	} while ((count > 0) && (inb_p(sp->io + UART_LSR) & UART_LSR_DR));

	return 0;
}

int rs_init()
{

	register struct serial_info *sp=&ports[0];
	int i;
	int ttyno = 4;
	printk("Serial driver version 0.01 with no serial options enabled\n");
	for(i=0;i<4;i++)
	{
		if((rs_probe(sp)==0) && (!request_irq(sp->irq, rs_irq, 0L, "serial")))
		{
			printk("ttys%d at 0x%x (irq = %d)",
				i,sp->io,sp->irq);
			switch(sp->flags&SERF_TYPE)
			{
				case ST_8250:
					printk(" is a 8250\n");
					break;
				case ST_16450:
					printk(" is a 16450\n");
					break;
				case ST_16550:
					printk(" is a 16550\n");
					break;
				default:
					printk("\n");
					break;
			}
			sp->tty = &ttys[ttyno++];
/*			outb_p(????, sp->io + UART_MCR); */
		}	
		sp++;
	}
	return 0;
}

#ifdef CONFIG_CONSOLE_SERIAL
static int con_init = 0;
void init_console()
{
	rs_init();
	con_init = 1;
	printk("Console: Serial\n");
}

void con_charout(Ch)
char Ch;
{
	if (con_init) {
		while (!(inb_p(ports[CONSOLE_PORT].io + UART_LSR) & UART_LSR_TEMT));
		outb(Ch, ports[CONSOLE_PORT].io + UART_TX);
	}
}

int wait_for_keypress()
{
	/* Do something */
}
#endif /* CONFIG_CONSOLE_SERIAL */
