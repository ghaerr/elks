#include <linuxmt/types.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/serial_reg.h>
#include <linuxmt/ntty.h>
#include <linuxmt/debug.h>
#include <linuxmt/mem.h>

#asm
entry _mod_rs_init
#endasm

extern struct tty ttys[];

struct serial_info
{
	unsigned short io;
	unsigned char irq;
	unsigned char flags;
	struct tty * tty;	
#define SERF_TYPE	15
#define SERF_EXIST	16
#define ST_8250		0
#define ST_16450	1
#define ST_16550	2
#define ST_UNKNOWN	15
};

#define RS_MINOR_OFFSET 4

static struct serial_info ports[4]=
{
	{ 0x3f8,4,0,0 },
	{ 0x2f8,3,0,0 },
	{ 0x3e8,5,0,0 },
	{ 0x2e8,2,0,0 },
};

static char irq_port[4] = {3,1,0,2};

static int rs_in_use[4];

int rs_open(inode,file);
void rs_release(inode,file);
int rs_write(tty);

struct tty_ops rs_ops = {
	rs_open,
	rs_release,
	rs_write,
	NULL,
	NULL,
};

int mod_rs_init(init_flag)
int init_flag;
{
	int i;

	if (init_flag == MOD_INIT) {
		printk("Serial Module.\n");
		return rs_init();
	} else {
		printk("Serial Module removed.\n");
		for (i = 4; i < 8; i++) {
			ttys[i].ops = NULL;
			ttys[i].minor = 0;
			if (ports[i -4].tty != NULL) {
				free_irq(ports[i-4].irq);
			}
		}
	}
}
	
int rs_open(inode,file)
struct inode *inode;
struct file *file;
{
	int rs_minor = MINOR(inode->i_rdev) - RS_MINOR_OFFSET;

	printd_rs("RS_OPEN called\n");
	if (!(ports[rs_minor].flags & SERF_EXIST))
		return -ENODEV;
	if (!rs_in_use[rs_minor]) {
		rs_in_use[rs_minor] = 1;
		inb_p(ports[rs_minor].io + UART_LSR);
		inb_p(ports[rs_minor].io + UART_RX);
		inb_p(ports[rs_minor].io + UART_IIR);
		inb_p(ports[rs_minor].io + UART_MSR);
		outb_p(UART_IER_RDI, ports[rs_minor].io + UART_IER);
		outb_p(UART_LCR_WLEN8, ports[rs_minor].io + UART_LCR);
		outb_p(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2, ports[rs_minor].io + UART_MCR);
		inb_p(ports[rs_minor].io + UART_LSR);
		inb_p(ports[rs_minor].io + UART_RX);
		inb_p(ports[rs_minor].io + UART_IIR);
		inb_p(ports[rs_minor].io + UART_MSR);
		
	} else return -EBUSY;
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

void receive_chars(sp)
struct serial_info *sp;
{
	char ch;

	do {
		ch = inb_p(sp->io + UART_RX);
		chq_addch(sp->tty->inq, ch, 0);
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
	struct serial_info * sp;
	unsigned char ch;
	printd_rs("Serial interrupt recieved.\n");

	sp = &ports[irq_port[irq -2]];
	do {
		status = inb_p(sp->io + UART_LSR);
		if (status & UART_LSR_DR)
			receive_chars(sp);
/*		if (status & UART_LSR_THRE)
			transmit_chars(sp); */
	} while (!(inb_p(sp->io + UART_IIR) & UART_IIR_NO_INT));
	
}

int rs_probe(sp)
struct serial_info *sp;
{
	int scratch,scratch2;
	int status1,status2;
	
	scratch=inb(sp->io+UART_IER);
	outb_p(0,sp->io+UART_IER);
	scratch=inb_p(sp->io+UART_IER);
	outb_p(scratch,sp->io+UART_IER);
	if(scratch)
		return -1;
	
	scratch2=inb_p(sp->io+UART_LCR);
	outb_p(scratch2 | UART_LCR_DLAB, sp->io+UART_LCR);
	outb_p(0,sp->io+UART_EFR);
	outb_p(scratch2,sp->io+UART_LCR);
	outb_p(UART_FCR_ENABLE_FIFO, sp->io+UART_FCR);
	scratch=inb_p(sp->io+UART_IIR)>>6;
	switch(scratch)
	{
		case 0:
			sp->flags=SERF_EXIST|ST_16450;
			break;
		case 1:
			sp->flags=ST_UNKNOWN;
			break;
		case 2:
		case 3:	/* Could be a 16650.. we dont care */
			sp->flags=SERF_EXIST|ST_16550;
			break;
	}
	
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
	outb_p(UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT,sp->io+UART_FCR);
	inb_p(sp->io+UART_RX);
	return 0;
}

int rs_init()
{
	struct serial_info *sp=&ports[0];
	int i;
	int ttyno = 4;
	printk("Serial driver version 0.01 with no serial options enabled\n");
	for(i=0;i<4;i++)
	{
		if((rs_probe(sp)==0) && (!request_irq(sp->irq, rs_irq)))
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
	for (i = 4; i < 8; i++) {
		ttys[i].ops = &rs_ops;
		ttys[i].minor = i;
	}
	return 0;
}


