#include <linuxmt/types.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/serial_reg.h>
#include <linuxmt/ntty.h>
#include <linuxmt/termios.h>
#include <linuxmt/debug.h>
#include <arch/io.h>
#include <arch/ports.h>

#define NEW		1	/* set =0 for old driver code if needed*/
#define FIFO		1	/* set =1 to enable new FIFO code for testing*/

#if defined (CONFIG_CHAR_DEV_RS) || defined (CONFIG_CONSOLE_SERIAL)

extern struct tty ttys[];

struct serial_info {
             char *io;
    unsigned char irq;
    unsigned char flags;
    unsigned char lcr;
    unsigned char mcr;
    unsigned int  divisor;
    unsigned char usecount;
    struct tty *tty;

#define SERF_TYPE	15
#define SERF_EXIST	16
#define ST_8250		0
#define ST_16450	1
#define ST_16550	2
#define ST_16550A	3
#define ST_16750	4
#define ST_UNKNOWN	15

};

#define CONSOLE_PORT 0

/* all boxes should be able to do 9600 at least,
 * afaik 8250 works fine up to 19200
 */

#define DEFAULT_BAUD_RATE	9600
#define DEFAULT_LCR		UART_LCR_WLEN8

#define DEFAULT_MCR		\
	((unsigned char) (UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2))

#define MAX_RX_BUFFER_SIZE 16

static struct serial_info ports[NR_SERIAL] = {
    {(char *)COM1_PORT, COM1_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, 0, NULL},
    {(char *)COM2_PORT, COM2_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, 0, NULL},
    {(char *)COM3_PORT, COM3_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, 0, NULL},
    {(char *)COM4_PORT, COM4_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, 0, NULL},
};

static char irq_port[NR_SERIAL] = { 3, 1, 0, 2 };

static unsigned int divisors[] = {
    0,				/*  0 = B0      */
    2304,			/*  1 = B50     */
    1536,			/*  2 = B75     */
    1047,			/*  3 = B110    */
    860,			/*  4 = B134    */
    768,			/*  5 = B150    */
    576,			/*  6 = B200    */
    384,			/*  7 = B300    */
    192,			/*  8 = B600    */
    96,				/*  9 = B1200   */
    64,				/* 10 = B1800   */
    48,				/* 11 = B2400   */
    24,				/* 12 = B4800   */
    12,				/* 13 = B9600   */
    6,				/* 14 = B19200  */
    3,				/* 15 = B38400  */
    2,				/* 16 = B57600  */
    1,				/* 17 = B115200 */
    0				/*  0 = B230400 */
};

/* Flow control buffer markers */
#define	RS_IALLMOSTFULL 	(3 * INQ_SIZE / 4)
#define	RS_IALLMOSTEMPTY	(    INQ_SIZE / 4)

/* Flush input fifo */
static void flush_input_fifo(register struct serial_info *sp)
{
#if !FIFO	/* HW FIFO cleared when reset or enabled*/
    int i = MAX_RX_BUFFER_SIZE;

    do {
	inb_p(sp->io + UART_RX);
    } while (--i && (inb_p(sp->io + UART_LSR) & UART_LSR_DR));
#endif
}

static int rs_probe(register struct serial_info *sp)
{
    int status, type;
    unsigned char scratch;

    inb(sp->io + UART_IER);
    outb_p(0, sp->io + UART_IER);
    scratch = inb_p(sp->io + UART_IER);
    outb_p(scratch, sp->io + UART_IER);
    if (scratch)
	return -1;

    /* try to enable 64 byte FIFO and max trigger*/
    outb_p(0xE7, sp->io + UART_FCR);

    /* then read FIFO status*/
    status = inb_p(sp->io + UART_IIR);
    if (status & 0x80) {		/* FIFO available*/
	if (status & 0x20)		/* 64 byte FIFO enabled*/
	    type = ST_16750;
	else if (status & 0x40)		/* 16 byte FIFO OK */
	    type = ST_16550A;
	else
	    type = ST_16550;		/* Non-functional FIFO */
    } else {
	/* no FIFO, try writing arbitrary value to scratch reg*/
	outb_p(0x2A, sp->io + UART_SCR);
	if (inb_p(sp->io + UART_SCR) == 0x2a)
	    type = ST_16450;
	else type = ST_8250;
    }

    sp->flags = SERF_EXIST | type;

    /*
     *      Reset the chip
     */
    outb_p(0x00, sp->io + UART_MCR);

    /* FIFO off, clear RX and TX FIFOs */
    outb_p(UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, sp->io + UART_FCR);

    /* clear RX register */
    flush_input_fifo(sp);

    return 0;
}

static void update_port(register struct serial_info *port)
{
    tcflag_t cflags;
    unsigned divisor;

    /* set baud rate divisor, first lower, then higher byte */
    cflags = port->tty->termios.c_cflag & CBAUD;
    if (cflags & CBAUDEX)
	cflags = B38400 + (cflags & 03);
    divisor = divisors[cflags];

    //FIXME: update lcr parity and data width from termios values

    /* update divisor only if changed, since we have not TCSETW*/
    if (divisor != port->divisor) {
	port->divisor = divisor;

	clr_irq();

	/* Set the divisor latch bit */
	outb_p(port->lcr | UART_LCR_DLAB, port->io + UART_LCR);

	/* Set the divisor low and high byte */
	outb_p((unsigned char)divisor, port->io + UART_DLL);
	outb_p((unsigned char)(divisor >> 8), port->io + UART_DLM);

	/* Clear the divisor latch bit */
	outb_p(port->lcr, port->io + UART_LCR);

	set_irq();
    }
}

/* WARNING: Polling write function */
static int rs_write(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    register char *i;

    i = 0;
    while (tty->outq.len > 0) {
	do {				/* Wait until transmitter buffer empty */
	} while (!(inb_p(port->io + UART_LSR) & UART_LSR_TEMT));
	outb((char)tty_outproc(tty), port->io + UART_TX);
	i++;				/* Write data to transmit buffer */
    }
    return (int)i;
}

#if NEW
void rs_irq(int irq, struct pt_regs *regs, void *dev_id)
{
    struct serial_info *sp;
    struct ch_queue *q;
    int i, j, status;
    char *io;
    unsigned char buf[MAX_RX_BUFFER_SIZE];

    i = 0;
    sp = &ports[(int)irq_port[irq - 2]];
    io = sp->io;

    status = inb_p(io + UART_LSR);			/* check for data overrun*/
    //if ((status & UART_LSR_DR) == 0)			/* QEMU may interrupt w/no data*/
	//return;

    /* empty fifo from uart into temp buffer*/
    //do {
	//status = inb_p(io + UART_LSR);
	//if (status & UART_LSR_DR)			/* Receiver buffer full? */
	    do {
		buf[i++] = inb_p(io + UART_RX);		/* Read received data */
	    } while ((inb_p(io + UART_LSR) & UART_LSR_DR) && i < MAX_RX_BUFFER_SIZE);
	//}
    //} while (!(inb_p(io + UART_IIR) & UART_IIR_NO_INT) && i < MAX_RX_BUFFER_SIZE);

    if (status & UART_LSR_OE)
	printk("serial: data overrun\n");
    /* QEMU sometimes has interrupt w/o data available bit set*/
    //if ((status & UART_LSR_DR) == 0)
	//printk("serial: interrupt w/o data available\n");

    /* process received chars*/
    q = &sp->tty->inq;
    for (j=0; j < i; j++) {
	if (!tty_intcheck(sp->tty, buf[j]))
	    chq_addch(q, buf[j]);			/* Save data in queue */
    }
    wake_up(&q->wait);
}

#else

static void receive_chars(register struct serial_info *sp)
{
    register struct ch_queue *q;
    unsigned char ch;

    q = &sp->tty->inq;
    do {
	ch = inb_p(sp->io + UART_RX);		/* Read received data */
	if (!tty_intcheck(sp->tty, ch)) {
	    chq_addch(q, ch);			/* Save data in queue */
	}
    } while (inb_p(sp->io + UART_LSR) & UART_LSR_DR);
    wake_up(&q->wait);
}

void rs_irq(int irq, struct pt_regs *regs, void *dev_id)
{
    register struct serial_info *sp;
    int status;


    debug1("SERIAL: Interrupt %d received.\n", irq);
    sp = &ports[(int)irq_port[irq - 2]];
    do {
	status = inb_p(sp->io + UART_LSR);
	if (status & UART_LSR_DR)		/* Receiver buffer full? */
	    receive_chars(sp);
#if 0
	if (status & UART_LSR_THRE)		/* Transmitter buffer empty? */
	    transmit_chars(sp);
#endif
    } while (!(inb_p(sp->io + UART_IIR) & UART_IIR_NO_INT));
}
#endif /* old rs_irq function*/

static void rs_release(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];

    debug_tty("SERIAL close %d\n", current->pid);
    if (--port->usecount == 0) {
	outb_p(0, port->io + UART_IER);	/* Disable all interrupts */
	tty_freeq(tty);
    }
}

static int rs_open(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int err;

    debug_tty("SERIAL open %d\n", current->pid);

    if (!(port->flags & SERF_EXIST))
	return -ENODEV;

    /* increment use count, don't init if already open*/
    if (port->usecount++)
	return 0;

    err = tty_allocq(tty, RSINQ_SIZE, RSOUTQ_SIZE);
    if (err) {
	--port->usecount;
	return err;
    }

    /* clear RX buffer */
    inb_p(port->io + UART_LSR);

    /* Flush input fifo */
    flush_input_fifo(port);

#define UART_FCR_ENABLE_FIFO14	(UART_FCR_ENABLE_FIFO | 0xC0)
#define UART_FCR_ENABLE_FIFO8	(UART_FCR_ENABLE_FIFO | 0x80)
#define UART_FCR_ENABLE_FIFO4	(UART_FCR_ENABLE_FIFO | 0x40)
    /* enable FIFO with 14 byte trigger */
#if FIFO
    if ((port->flags & SERF_TYPE) > ST_16550)
	outb_p(UART_FCR_ENABLE_FIFO14, port->io + UART_FCR);
#endif

    inb_p(port->io + UART_IIR);
    inb_p(port->io + UART_MSR);

    /* set serial port parameters to match ports[rs_minor] */
    update_port(port);

    /* enable receiver data interrupt*/
    outb_p(UART_IER_RDI, port->io + UART_IER);

    outb_p(port->mcr, port->io + UART_MCR);

    /* clear Line/Modem Status, Intr ID and RX register */
    inb_p(port->io + UART_LSR);
    inb_p(port->io + UART_RX);
    inb_p(port->io + UART_IIR);
    inb_p(port->io + UART_MSR);

    return 0;
}

#if 0	// unused, removed for bloat
static int set_serial_info(struct serial_info *info,
			   struct serial_info *new_info)
{
    register struct tty *tty;
    int err;

    tty = info->tty;
    err = verified_memcpy_fromfs(info, new_info, sizeof(struct serial_info));
    if (!err) {
	/* shutdown serial port and restart UART with new settings */

	/* witty cheat :) - either we do this (and waste some DS) or duplicate
	 * almost whole rs_release and rs_open (and waste much more CS)
	 */
	info->tty = tty;
	rs_release(tty);
	err = rs_open(tty);
    }
    return err;
}

static int get_serial_info(struct serial_info *info,
			   struct serial_info *ret_info)
{
    return verified_memcpy_tofs(ret_info, info, sizeof(struct serial_info));
}
#endif

static int rs_ioctl(struct tty *tty, int cmd, char *arg)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int retval = 0;

    /* few sanity checks should be here */
    debug2("rs_ioctl: sp = %d, cmd = %d\n", tty->minor - RS_MINOR_OFFSET, cmd);
    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
	/* verified_memcpy*fs() already called by ntty.c ioctl handler*/
	//FIXME: update_port() only sets baud rate from termios, not parity or wordlen*/
	update_port(port);	/* ignored return value*/
	break;
#if 0
    case TIOCSSERIAL:
	retval = set_serial_info(port, (struct serial_info *)arg);
	break;

    case TIOCGSERIAL:
	retval = get_serial_info(port, (struct serial_info *)arg);
#endif

    default:
	return -EINVAL;
    }

    return retval;
}

int rs_init(void)
{
    register struct serial_info *sp = ports;
    register struct tty *tty = ttys + NR_CONSOLES;
    int ttyno = 0;
    static char *serial_type[] = {
	"n 8250",
	" 16450",
	" 16550",
	" 16550A",
	" 16750",
	" UNKNOWN",
    };

    do {
	if (!rs_probe(sp) && !request_irq(sp->irq, rs_irq, NULL)) {
	    sp->tty = tty;
	    update_port(sp);
	}
	tty++;
    } while (++sp < &ports[NR_SERIAL]);

    sp = ports;
    do {
	if (sp->tty != NULL) {
	    printk("ttyS%d at 0x%x, irq %d is a%s\n", ttyno,
		       sp->io, sp->irq, serial_type[sp->flags & SERF_TYPE]);
	}
	sp++;
    } while (++ttyno < NR_SERIAL);
    return 0;
}

#ifdef CONFIG_CONSOLE_SERIAL

static int con_init = 0;

void init_console(void)
{
    register struct serial_info *sp = &ports[CONSOLE_PORT];

    rs_init();
    memcpy((void *)&(sp->tty->termios), &def_vals, sizeof(struct termios));
    update_port(sp);
    con_init = 1;
    printk("Console: Serial\n");
}

void con_charout(char Ch)
{
    if (con_init) {
	while (!(inb_p(ports[CONSOLE_PORT].io + UART_LSR) & UART_LSR_TEMT));
	outb(Ch, ports[CONSOLE_PORT].io + UART_TX);
    }
}

int wait_for_keypress(void)
{
    /* FIXME*/
    return '\n';
}

#endif

struct tty_ops rs_ops = {
    rs_open,
    rs_release,
    rs_write,
    NULL,
    rs_ioctl
};

#endif
