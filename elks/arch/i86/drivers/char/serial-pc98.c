/*
 * PC-98 Serial Driver for ELKS
 * (T.Yamada used template by Greg Haerr)
 *
 * Supports PC-98 UARTs, fixed configuration, no FIFO, no probing.
 * 1 serial port.
 *
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
#include <arch/irq.h>
#include <arch/serial-pc98.h>	/* UART-specific header*/
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

/* Timer 2 mode definitions */
#define TIMER2_MODE3 0xB6   /* timer 2, binary count, mode 3, lsb/msb */

/* UART-specific definititions for line and modem control register*/
#define DEFAULT_LCR		UART_LCR_STOP1 | UART_LCR_WLEN8 | UART_LCR_MODE16

static struct serial_info ports[NR_SERIAL] = {
    {COM1_PORT, COM1_IRQ, 0, DEFAULT_LCR, 0, 0, NULL}
};

/* UART clock divisors per baud rate*/
/* For 5MHz system, 1/16 mode */
static unsigned int divisors_5MHz[] = {
    0,				/*  0 = B0      */
    0,				/*  1 = B50     */
    0,				/*  2 = B75     */
    0,				/*  3 = B110    */
    0,				/*  4 = B134    */
    0,				/*  5 = B150    */
    0,				/*  6 = B200    */
    512,			/*  7 = B300    */
    256,			/*  8 = B600    */
    128,			/*  9 = B1200   */
    0,				/* 10 = B1800   */
    64,				/* 11 = B2400   */
    32,				/* 12 = B4800   */
    16,				/* 13 = B9600   */
    8,				/* 14 = B19200  */
    4,				/* 15 = B38400  */
    0,				/* 16 = B57600  */
    0,				/* 17 = B115200 */
    0				/*  0 = B230400 */
};

/* For 8MHz system, 1/16 mode */
static unsigned int divisors_8MHz[] = {
    0,				/*  0 = B0      */
    0,				/*  1 = B50     */
    0,				/*  2 = B75     */
    0,				/*  3 = B110    */
    0,				/*  4 = B134    */
    0,				/*  5 = B150    */
    0,				/*  6 = B200    */
    416,			/*  7 = B300    */
    208,			/*  8 = B600    */
    104,			/*  9 = B1200   */
    0,				/* 10 = B1800   */
    52,				/* 11 = B2400   */
    26,				/* 12 = B4800   */
    13,				/* 13 = B9600   */
    0,				/* 14 = B19200  */
    0,				/* 15 = B38400  */
    0,				/* 16 = B57600  */
    0,				/* 17 = B115200 */
    0				/*  0 = B230400 */
};

extern struct tty ttys[];

static int rs_init_done = 0;

/* serial write - busy loops until transmit buffer available */
static int rs_write(struct tty *tty)
{
    struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int i = 0;

    while (tty->outq.len > 0) {
	unsigned long timeout = jiffies + 4*HZ/100; /* 40ms, 300 baud needs 33.3ms */
	/* Wait until transmitter hold buffer empty */
	while (!(inb(port->io + UART_LSR) & UART_LSR_THRE)) {
	    if (time_after(jiffies, timeout)) /* waits 40ms max, jiffies updated by hw timer */
	        break;
	}
	outb((char)tty_outproc(tty), port->io + UART_TX);
	i++;
    }
    return i;
}

/* serial interrupt routine, reads and queues received character */
void rs_irq(int irq, struct pt_regs *regs)
{
    /* below line allows for max one serial port */
    struct serial_info *sp = ports;
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
	outb(inb(port->io + UART_IER) & 0xF8, port->io + UART_IER);	/* Disable all interrupts */
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

    if (peekb(0x501, 0) & 0x80) {
	/* 8MHz system */
	divisor = divisors_8MHz[cflags];
    } else {
	/* 5MHz system */
	divisor = divisors_5MHz[cflags];
    }

    /* FIXME: should update lcr parity and data width from termios values*/

    /* update divisor only if changed, since we have not TCSETW*/
    if (!divisor) {
	printk("serial: baud rate not supported.\n");
    }
    else if (divisor != port->divisor) {
	port->divisor = divisor;

	clr_irq();

	outb(TIMER2_MODE3, TIMER_CMDS_PORT);

	/* Set the divisor low and high byte */
	outb((unsigned char)divisor, TIMER2_PORT);          /* LSB */
	outb((unsigned char)(divisor >> 8), TIMER2_PORT);   /* MSB */

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
    inb(port->io + UART_MSR);

    /* set serial port parameters to match ports[rs_minor] */
    update_port(port);

    /* enable receiver data interrupt*/
    outb(inb(port->io + UART_IER) | UART_IER_RDI, port->io + UART_IER);

    /* clear Line/Modem Status, Intr ID and RX register */
    inb(port->io + UART_LSR);
    inb(port->io + UART_RX);
    inb(port->io + UART_MSR);

    return 0;
}

/* note: this function will be called prior to serial_init if serial console set*/
void rs_conout(dev_t dev, int c)
{
    struct serial_info *sp = &ports[MINOR(dev) - RS_MINOR_OFFSET];
    unsigned long timeout = jiffies + 4*HZ/100; /* 40ms, 300 baud needs 33.3ms */

    while (!(inb(sp->io + UART_LSR) & UART_LSR_THRE)) {
	if (time_after(jiffies, timeout)) /* waits 40ms max, jiffies updated by hw timer */
	    break;
    }
    outb(c, sp->io + UART_TX);
}

/* initialize UART */
static void rs_init(struct serial_info *sp)
{
    /* Reset */
    /* Set the mode register */
    outb(0x8E, sp->io + UART_LCR); // dummy, bit6 must be 0
    outb(0x8E, sp->io + UART_LCR); // dummy, bit6 must be 0
    outb(0x8E, sp->io + UART_LCR); // dummy, bit6 must be 0
    /* Set the command word */
    outb(0x8E | UART_LCR_RESET, sp->io + UART_LCR); // reset, set bit6

    /* Set the mode register */
    outb(sp->lcr, sp->io + UART_LCR);

    /* Set the command word */
    outb(UART_LCR_RTS_ON | UART_LCR_RX_EN | UART_LCR_DTR_ON | UART_LCR_TX_EN, sp->io + UART_LCR);
}

void INITPROC serial_init(void)
{
    struct serial_info *sp;
    struct tty *tty = ttys + NR_CONSOLES;

    for (sp = ports; sp < &ports[NR_SERIAL]; sp++, tty++) {
	/* If rs_init is not done at rs_setbaud for serial console, do it here */
	if (!rs_init_done)
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

	rs_init(sp);
	rs_init_done = 1;

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
