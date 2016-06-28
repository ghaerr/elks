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

#if defined (CONFIG_CHAR_DEV_RS) || defined (CONFIG_CONSOLE_SERIAL)

extern struct tty ttys[];

struct serial_info {
             char *io;
    unsigned char irq;
    unsigned char flags;
    unsigned char lcr;
    unsigned char mcr;
    struct tty *tty;

#define SERF_TYPE	15
#define SERF_EXIST	16
#define SERF_INUSE	32
#define ST_8250		0
#define ST_16450	1
#define ST_16550	2
#define ST_UNKNOWN	15

};

#define RS_MINOR_OFFSET 64

#define CONSOLE_PORT 0

/* all boxes should be able to do 9600 at least,
 * afaik 8250 works fine up to 19200
 */

#define DEFAULT_BAUD_RATE	9600
#define DEFAULT_LCR		UART_LCR_WLEN8

#define DEFAULT_MCR		\
	((unsigned char) (UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2))

#define MAX_RX_BUFFER_SIZE 16

static struct serial_info ports[4] = {
    {(char *)0x3f8, 4, 0, DEFAULT_LCR, DEFAULT_MCR, NULL},
    {(char *)0x2f8, 3, 0, DEFAULT_LCR, DEFAULT_MCR, NULL},
    {(char *)0x3e8, 5, 0, DEFAULT_LCR, DEFAULT_MCR, NULL},
    {(char *)0x2e8, 2, 0, DEFAULT_LCR, DEFAULT_MCR, NULL},
};

static char irq_port[4] = { 3, 1, 0, 2 };

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

static int rs_probe(register struct serial_info *sp)
{
    int status1, status2;
    register char *scratch;

    inb(sp->io + UART_IER);
    outb_p(0, sp->io + UART_IER);
    scratch = (char *)inb_p(sp->io + UART_IER);
    outb_p((unsigned char)scratch, sp->io + UART_IER);
    if ((unsigned char)scratch)
	return -1;

    /* this code is weird, IMO */
    scratch = (char *)inb_p(sp->io + UART_LCR);
    outb_p((unsigned char)scratch | UART_LCR_DLAB, sp->io + UART_LCR);
    outb_p(0, sp->io + UART_EFR);
    outb_p((unsigned char)scratch, sp->io + UART_LCR);

    outb_p(UART_FCR_ENABLE_FIFO, sp->io + UART_FCR);

    /* upper two bits of IIR define UART type, but according to both RB's
     * intlist and HelpPC this code is wrong, see comments marked with [*]
     */
    scratch = (char *)(inb_p(sp->io + UART_IIR) >> 6);
    switch ((unsigned char)scratch) {
    case 0:
	sp->flags = (unsigned char) (SERF_EXIST | ST_16450);
	break;
    case 1:			/* [*] this denotes broken 16550 UART,
				 * not 16550A or any newer type */
	sp->flags = (unsigned char) (ST_UNKNOWN);
	break;
    case 2:			/* invalid combination */
    case 3:			/* Could be a 16650.. we dont care */
	/* [*] 16550A or newer with enabled FIFO buffers
	 */
	sp->flags = (unsigned char) (SERF_EXIST | ST_16550);
	break;
    }

    /* 8250 UART if scratch register isn't present */
    if (!(unsigned char)scratch) {
	scratch = (char *)inb_p(sp->io + UART_SCR);
	outb_p(0xA5, sp->io + UART_SCR);
	status1 = inb_p(sp->io + UART_SCR);
	outb_p(0x5A, sp->io + UART_SCR);
	status2 = inb_p(sp->io + UART_SCR);
	outb_p((unsigned char)scratch, sp->io + UART_SCR);
	if ((status1 != 0xA5) || (status2 != 0x5A))
	    sp->flags = (unsigned char) (SERF_EXIST | ST_8250);
    }

    /*
     *      Reset the chip
     */

    outb_p(0x00, sp->io + UART_MCR);

    /* clear RX and TX FIFOs */
    outb_p((unsigned char) (UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT),
			sp->io + UART_FCR);

    /* clear RX register */
    scratch = (char *) MAX_RX_BUFFER_SIZE;
    do {
	inb_p(sp->io + UART_RX);		/* Flush input fifo */
    } while (--scratch && (inb_p(sp->io + UART_LSR) & UART_LSR_DR));

    return 0;
}

static void update_port(register struct serial_info *port)
{
    tcflag_t cflags;
    unsigned divisor;

    /* set baud rate divisor, first lower, then higher byte */
    cflags = port->tty->termios.c_cflag & CBAUD;
    if(cflags & CBAUDEX)
	cflags = B38400 + cflags & 03;
    divisor = divisors[cflags];

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

static int rs_write(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    register char *i;

    i = 0;
    while (tty->outq.len > 0) {
	do {				/* Wait until transmitter buffer empty */
	} while(!(inb_p(port->io + UART_LSR) & UART_LSR_TEMT));
	outb((char)tty_outproc(tty), port->io + UART_TX);
	i++;				/* Write data to transmit buffer */
    }
    return (int)i;
}

static void receive_chars(register struct serial_info *sp)
{
    register struct ch_queue *q;
    unsigned char ch;

    q = &sp->tty->inq;
    do {
	ch = inb_p(sp->io + UART_RX);		/* Read received data */
	if (!tty_intcheck(sp->tty, ch)) {
	    chq_addch(q, ch, 0);		/* Save data in queue */
	}
    } while (inb_p(sp->io + UART_LSR) & UART_LSR_DR);
    wake_up(&q->wq);
}

void rs_irq(int irq, struct pt_regs *regs, void *dev_id)
{
    register struct serial_info *sp;
    register char *statusp;


    debug1("SERIAL: Interrupt %d recieved.\n", irq);
    sp = &ports[(int)irq_port[irq - 2]];
    do {
	statusp = (char *)inb_p(sp->io + UART_LSR);
	if ((int)statusp & UART_LSR_DR)		/* Receiver buffer full? */
	    receive_chars(sp);
#if 0
	if (((int)statusp) & UART_LSR_THRE)	/* Transmitter buffer empty? */
	    transmit_chars(sp);
#endif
    } while (!(inb_p(sp->io + UART_IIR) & UART_IIR_NO_INT));
}

static void rs_release(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];

    debug("SERIAL: rs_release called\n");
    port->flags &= ~SERF_INUSE;
    outb_p(0, port->io + UART_IER);	/* Disable all interrupts */
}

static int rs_open(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    register char *countp;

    debug("SERIAL: rs_open called\n");

    if (!(port->flags & SERF_EXIST))
	return -ENODEV;

    /* is port already in use ? */
    if (port->flags & SERF_INUSE)
	return -EBUSY;

    /* no, mark it in use */
    port->flags |= SERF_INUSE;

    /* clear RX buffer */
    inb_p(port->io + UART_LSR);

    countp = (char *) MAX_RX_BUFFER_SIZE;
    do
	inb_p(port->io + UART_RX);		/* Flush input fifo */
    while (--countp && (inb_p(port->io + UART_LSR)) & UART_LSR_DR);

    inb_p(port->io + UART_IIR);
    inb_p(port->io + UART_MSR);

    /* set serial port parameters to match ports[rs_minor] */
    update_port(port);

    /* enable receiver data interrupt; FIXME: update code to utilize full interrupt interface */
    outb_p(UART_IER_RDI, port->io + UART_IER);

    outb_p(port->mcr, port->io + UART_MCR);

    /* clear Line/Modem Status, Intr ID and RX register */
    inb_p(port->io + UART_LSR);
    inb_p(port->io + UART_RX);
    inb_p(port->io + UART_IIR);
    inb_p(port->io + UART_MSR);

    return 0;
}

static int set_serial_info(struct serial_info *info,
			   struct serial_info *new_info)
{
    register struct tty *tty;
    register char *errp;

    tty = info->tty;
    errp = (char *) verified_memcpy_fromfs(info, new_info,
					   sizeof(struct serial_info));
    if (!errp) {
	/* shutdown serial port and restart UART with new settings */

	/* witty cheat :) - either we do this (and waste some DS) or duplicate
	 * almost whole rs_release and rs_open (and waste much more CS)
	 */
	info->tty = tty;
	rs_release(tty);
	errp = (char *) rs_open(tty);
    }
    return (int) errp;
}

static int get_serial_info(struct serial_info *info,
			   struct serial_info *ret_info)
{
    return verified_memcpy_tofs(ret_info, info, sizeof(struct serial_info));
}

static int rs_ioctl(struct tty *tty, int cmd, char *arg)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    register char *retvalp = 0;

    /* few sanity checks should be here */
    debug2("rs_ioctl: sp = %d, cmd = %d\n", tty->minor - RS_MINOR_OFFSET, cmd);
    switch (cmd) {
	/* Unlike Linux we use verified_memcpy*fs() which calls verify_area() for us */
    case TCSETS:
    case TCSETSW:
    case TCSETSF:		/* For information, return value is ignored */
	update_port(port);
	break;
    case TIOCSSERIAL:
	retvalp = (char *) set_serial_info(port, (struct serial_info *)arg);
	break;

    case TIOCGSERIAL:
	retvalp = (char *) get_serial_info(port, (struct serial_info *)arg);
	break;
    }

    return (int) retvalp;
}

int rs_init(void)
{
    register struct serial_info *sp = ports;
    int ttyno = 0;
    static char *serial_type[] = {
	"n 8250",
	" 16450",
	" 16550",
	" UNKNOWN",
    };

    printk("Serial driver version 0.02\n");
    do {
	if((sp->tty != NULL) || (!rs_probe(sp) && !request_irq(sp->irq, rs_irq, NULL))) {
	    printk("ttyS%d at 0x%x (irq = %d) is a%s%s\n", ttyno,
		       sp->io, sp->irq, serial_type[sp->flags & 0x3],
		       (sp->tty != NULL ? ", fetched" : ""));
	    if(sp->tty == NULL) {
		sp->tty = &ttys[4 + ttyno];
		update_port(sp);
#if 0
		outb_p(? ? ? ?, sp->io + UART_MCR);
#endif
	    }
	    ttyno++;
	}
    } while(++sp < &ports[4]);
    return 0;
}

#ifdef CONFIG_CONSOLE_SERIAL

static int con_init = 0;

void init_console(void)
{
    register struct serial_info *sp = &ports[CONSOLE_PORT];

    rs_init();
    memcpy((void *)&(sp->tty->termios),
		    &def_vals, sizeof(struct termios));
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
    /* Do something */
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
