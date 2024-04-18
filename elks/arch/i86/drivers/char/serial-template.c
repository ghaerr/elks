/*
 * ELKS Serial Driver (use as template for new serial drivers)
 *
 * Supports IBM PC UARTs, fixed configuration, no FIFO, no probing.
 * Set MAX_SERIAL in config.h for # of serial ports.
 *
 * May 2021 Greg Haerr
 */
#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/config.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/mm.h>
#include <linuxmt/ntty.h>
#include <linuxmt/kdev_t.h>
#include <linuxmt/termios.h>
#include <linuxmt/debug.h>
#include <arch/io.h>
#include <arch/serial-8250.h>	/* UART-specific header*/
#include <arch/ports.h>		/* definitions of COM1_PORT/COM1_IRQ*/

struct serial_info {
    unsigned int  io;
    unsigned char irq;
    unsigned char flags;
    unsigned char lcr;
    unsigned char mcr;
    unsigned int  divisor;
    struct tty *tty;
};

/* UART-specific definititions for line and modem control register*/
#define DEFAULT_LCR		UART_LCR_WLEN8
#define DEFAULT_MCR		(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2)

static struct serial_info ports[NR_SERIAL] = {
    {COM1_PORT, COM1_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, NULL},
    {COM2_PORT, COM2_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, NULL}
};

/* UART clock divisors per baud rate*/
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

extern struct tty ttys[];

/* serial write - busy loops until transmit buffer available */
static int rs_write(struct tty *tty)
{
    struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int i = 0;

    while (tty->outq.len > 0) {
	/* Wait until transmitter hold buffer empty */
	while (!(inb(port->io + UART_LSR) & UART_LSR_THRE))
		;
	outb((char)tty_outproc(tty), port->io + UART_TX);
	i++;
    }
    return i;
}

/* serial interrupt routine, reads and queues received character */
void rs_irq(int irq, struct pt_regs *regs)
{
    /* FIXME below line allows for max two serial ports */
    struct serial_info *sp = (irq == ports[0].irq)? ports: &ports[1];
    unsigned int io = sp->io;
    struct ch_queue *q = &sp->tty->inq;

    unsigned char c = inb(io + UART_RX);	/* Read received data */
    if (!tty_intcheck(sp->tty, c))
	chq_addch(q, c);
}


/* serial close */
static void rs_release(struct tty *tty)
{
    struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];

    debug_tty("SERIAL close %P\n");
    if (--tty->usecount == 0) {
	outb(0, port->io + UART_IER);	/* Disable all interrupts */
	tty_freeq(tty);
    }
}

/* update UART with current port termios settings*/
static void update_port(struct serial_info *port)
{
    unsigned int cflags;	/* use smaller 16-bit width to save code*/
    unsigned divisor;

    /* set baud rate divisor, first lower, then higher byte */
    cflags = port->tty->termios.c_cflag & CBAUD;
    if (cflags & CBAUDEX)
	cflags = B38400 + (cflags & 03);
    divisor = divisors[cflags];

    /* FIXME: should update lcr parity and data width from termios values*/

    /* update divisor only if changed, since we have not TCSETW*/
    if (divisor != port->divisor) {
	port->divisor = divisor;

	clr_irq();

	/* Set the divisor latch bit */
	outb(port->lcr | UART_LCR_DLAB, port->io + UART_LCR);

	/* Set the divisor low and high byte */
	outb((unsigned char)divisor, port->io + UART_DLL);
	outb((unsigned char)(divisor >> 8), port->io + UART_DLM);

	/* Clear the divisor latch bit */
	outb(port->lcr, port->io + UART_LCR);

	set_irq();
    }
}

/* serial open */
static int rs_open(struct tty *tty)
{
    struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int err;

    debug_tty("SERIAL open %P\n");

    /* increment use count, don't init if already open*/
    if (tty->usecount++)
	return 0;

    /* FIXME for now, use 80 rather than 1024 RSINQ_SIZE unless SLIP in use */
    err = tty_allocq(tty, 80, RSOUTQ_SIZE);
    if (err) {
	--tty->usecount;
	return err;
    }

    /* clear RX buffers */
    inb(port->io + UART_LSR);
    inb(port->io + UART_RX);
    inb(port->io + UART_IIR);
    inb(port->io + UART_MSR);

    /* set serial port parameters to match ports[rs_minor] */
    update_port(port);

    /* enable receiver data interrupt*/
    outb(UART_IER_RDI, port->io + UART_IER);

    outb(port->mcr, port->io + UART_MCR);

    /* clear Line/Modem Status, Intr ID and RX register */
    inb(port->io + UART_LSR);
    inb(port->io + UART_RX);
    inb(port->io + UART_IIR);
    inb(port->io + UART_MSR);

    return 0;
}

/* note: this function will be called prior to serial_init if serial console set*/
void rs_conout(dev_t dev, int c)
{
    struct serial_info *sp = &ports[MINOR(dev) - RS_MINOR_OFFSET];

    while (!(inb(sp->io + UART_LSR) & UART_LSR_THRE))
	continue;
    outb(c, sp->io + UART_TX);
}

/* initialize UART, interrupts off */
static void rs_init(struct serial_info *sp)
{
    /* reset chip */
    outb(0x00, sp->io + UART_MCR);

    /* FIFO off, clear RX and TX FIFOs */
    outb(UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, sp->io + UART_FCR);

    /* clear RX register */
}

void INITPROC serial_init(void)
{
    struct serial_info *sp;
    struct tty *tty = ttys + NR_CONSOLES;

    for (sp = ports; sp < &ports[NR_SERIAL]; sp++, tty++) {
	rs_init(sp);
	if (request_irq(sp->irq, rs_irq, INT_GENERIC))
	    printk("Can't get serial IRQ %d\n", sp->irq);
	else {
	    sp->tty = tty;
	    printk("ttyS%d at %x, irq %d\n", sp-ports, sp->io, sp->irq);
	    update_port(sp);
	}
    }
}

static int rs_ioctl(struct tty *tty, int cmd, char *arg)
{
    struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];

    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
	update_port(port);
	break;

    default:
	return -EINVAL;
    }
    return 0;
}

#ifdef CONFIG_BOOTOPTS
/* note: this function may be called prior to serial_init if serial console set*/
void INITPROC rs_setbaud(dev_t dev, unsigned long baud)
{
    struct serial_info *sp = &ports[MINOR(dev) - RS_MINOR_OFFSET];
    struct tty *tty = ttys + NR_CONSOLES + MINOR(dev) - RS_MINOR_OFFSET;
    unsigned int b;	/* use smaller 16-bit width to reduce code*/

	if (baud == 115200) b = B115200;
	else if ((unsigned)baud == 57600) b = B57600;
	else if ((unsigned)baud == 38400) b = B38400;
	else if ((unsigned)baud == 19200) b = B19200;
	else if ((unsigned)baud == 9600) b = B9600;
	else if ((unsigned)baud == 4800) b = B4800;
	else if ((unsigned)baud == 2400) b = B2400;
	else if ((unsigned)baud == 1200) b = B1200;
	else if ((unsigned)baud == 300) b = B300;
	else if ((unsigned)baud == 110) b = 110;
	else return;

    sp->tty = tty;	/* force tty association with serial port*/
    tty->termios.c_cflag = b | CS8;
    update_port(sp);
}
#endif

struct tty_ops rs_ops = {
    rs_open,
    rs_release,
    rs_write,
    NULL,
    rs_ioctl,
    rs_conout
};
