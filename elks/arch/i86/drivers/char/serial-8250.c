/*
 * IBM PC Compatible Serial Driver for ELKS
 *
 * Probes for specific UART at boot.
 * Supports 8250, 16450, 16550, 16550A and 16750 variants.
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
#include <arch/serial-8250.h>
#include <arch/ports.h>

struct serial_info {
             char *io;
    unsigned char irq;
    unsigned char flags;
    unsigned char lcr;
    unsigned char mcr;
    unsigned int  divisor;
    struct tty *tty;
    int pad1, pad2, pad3;       // round out to 16 bytes for faster addressing of ports[]
};

/* flags*/
#define SERF_TYPE       15
#define SERF_EXIST      16
#define ST_8250         0
#define ST_16450        1
#define ST_16550        2
#define ST_16550A       3
#define ST_16750        4
#define ST_UNKNOWN      15

#define CONSOLE_PORT 0

/* I/O delay settings*/
#define INB             inb     // use inb_p for 1us delay
#define OUTB            outb    // use outb_p for 1us delay

#define DEFAULT_LCR             UART_LCR_WLEN8

#define DEFAULT_MCR             \
        ((unsigned char) (UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2))

static struct serial_info ports[NR_SERIAL] = {
    {(char *)COM1_PORT, COM1_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, NULL, 0,0,0},
    {(char *)COM2_PORT, COM2_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, NULL, 0,0,0},
    {(char *)COM3_PORT, COM3_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, NULL, 0,0,0},
    {(char *)COM4_PORT, COM4_IRQ, 0, DEFAULT_LCR, DEFAULT_MCR, 0, NULL, 0,0,0},
};

static char irq_to_port[16];
static unsigned int divisors[] = {
    0,                          /*  0 = B0      */
    2304,                       /*  1 = B50     */
    1536,                       /*  2 = B75     */
    1047,                       /*  3 = B110    */
    860,                        /*  4 = B134    */
    768,                        /*  5 = B150    */
    576,                        /*  6 = B200    */
    384,                        /*  7 = B300    */
    192,                        /*  8 = B600    */
    96,                         /*  9 = B1200   */
    64,                         /* 10 = B1800   */
    48,                         /* 11 = B2400   */
    24,                         /* 12 = B4800   */
    12,                         /* 13 = B9600   */
    6,                          /* 14 = B19200  */
    3,                          /* 15 = B38400  */
    2,                          /* 16 = B57600  */
    1,                          /* 17 = B115200 */
    0                           /*  0 = B230400 */
};

extern struct tty ttys[];

/* Flow control buffer markers */
#define RS_IALLMOSTFULL         (3 * INQ_SIZE / 4)
#define RS_IALLMOSTEMPTY        (    INQ_SIZE / 4)

/* allow init to easily update the port irq from bootopts */
void set_serial_irq(int tty, int irq)
{
        ports[tty].irq = irq;
}

/*
 * Flush input by reading RX register.
 * Not used when FIFO enabled, as HW fifo cleared when enabled.
 */
static void flush_input(register struct serial_info *sp)
{
#ifndef CONFIG_HW_SERIAL_FIFO
    do {
        INB(sp->io + UART_RX);
    } while (INB(sp->io + UART_LSR) & UART_LSR_DR);
#endif
}

static int rs_probe(register struct serial_info *sp)
{
    int status, type;
    unsigned char scratch;

    INB(sp->io + UART_IER);
    OUTB(0, sp->io + UART_IER);
    scratch = INB(sp->io + UART_IER);
    OUTB(scratch, sp->io + UART_IER);
    if (scratch)
        return -1;

    /* try to enable 64 byte FIFO and max trigger*/
    OUTB(0xE7, sp->io + UART_FCR);

    /* then read FIFO status*/
    status = INB(sp->io + UART_IIR);
    if (status & 0x80) {                /* FIFO available*/
        if (status & 0x20)              /* 64 byte FIFO enabled*/
            type = ST_16750;
        else if (status & 0x40)         /* 16 byte FIFO OK */
            type = ST_16550A;
        else
            type = ST_16550;            /* Non-functional FIFO */
    } else {
        /* no FIFO, try writing arbitrary value to scratch reg*/
        OUTB(0x2A, sp->io + UART_SCR);
        if (INB(sp->io + UART_SCR) == 0x2a)
            type = ST_16450;
        else type = ST_8250;
    }

    sp->flags = SERF_EXIST | type;

    /*
     *      Reset the chip
     */
    OUTB(0x00, sp->io + UART_MCR);

    /* FIFO off, clear RX and TX FIFOs */
    OUTB(UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, sp->io + UART_FCR);

    /* clear RX register */
    flush_input(sp);

    return 0;
}

static void update_port(register struct serial_info *port)
{
    unsigned int cflags;        /* use smaller 16-bit width to save code*/
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
        OUTB(port->lcr | UART_LCR_DLAB, port->io + UART_LCR);

        /* Set the divisor low and high byte */
        OUTB((unsigned char)divisor, port->io + UART_DLL);
        OUTB((unsigned char)(divisor >> 8), port->io + UART_DLM);

        /* Clear the divisor latch bit */
        OUTB(port->lcr, port->io + UART_LCR);

        set_irq();
    }
}

/* serial write - busy loops until transmit buffer available */
static int rs_write(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int i = 0;

    while (tty->outq.len > 0) {
        /* Wait until transmitter hold buffer empty */
        while (!(INB(port->io + UART_LSR) & UART_LSR_THRE))
                ;
        outb((char)tty_outproc(tty), port->io + UART_TX);
        i++;
    }
    return i;
}

#if defined(CONFIG_FAST_IRQ4) || defined(CONFIG_FAST_IRQ3)
/* called from timer interrupt - check ring buffer and wakeup waiting processes*/
void rs_pump(void)
{
    struct serial_info *sp;
    struct ch_queue *q;

#ifdef CONFIG_FAST_IRQ4
    sp = &ports[0];
    q = &sp->tty->inq;

    if (sp->tty->usecount && q->len)
        wake_up(&q->wait);
#endif
#ifdef CONFIG_FAST_IRQ3
    sp = &ports[1];
    q = &sp->tty->inq;

    if (sp->tty->usecount && q->len)
        wake_up(&q->wait);
#endif
}
#endif

#ifdef CONFIG_FAST_IRQ4
/*
 * Fast serial driver for slower machines. Should work up to 38400 baud.
 * No ISIG (tty signal interrupt) handling for shells, used for fast SLIP transfer.
 *
 * Specially-coded fast C interrupt handler, called from asm irq_com[12] after saving
 * scratch registers AX,BX,CX,DX & DS and setting DS to kernel data segment.
 * NOTE: no parameters can be passed, nor any code written which
 * emits code using SP or BP addressing, as SS is not set and not guaranteed to equal DS.
 * Use 'ia16-elfk-objdump -D -r -Mi8086 serial.o' to look at code generated.
 */
extern void _irq_com1(int irq, struct pt_regs *regs);
void fast_com1_irq(void)
{
    struct serial_info *sp = &ports[0];
    char *io = sp->io;
    struct ch_queue *q = &sp->tty->inq;
    unsigned char c;

    c = INB(io + UART_RX);              /* Read received data */
    if (q->len < q->size) {
        q->base[q->head] = c;
        if (++q->head >= q->size)
            q->head = 0;
        q->len++;
    }
}
#endif

#ifdef CONFIG_FAST_IRQ3
extern void _irq_com2(int irq, struct pt_regs *regs);
void fast_com2_irq(void)
{
    struct serial_info *sp = &ports[1];
    char *io = sp->io;
    struct ch_queue *q = &sp->tty->inq;
    unsigned char c;

    c = INB(io + UART_RX);              /* Read received data */
    if (q->len < q->size) {
        q->base[q->head] = c;
        if (++q->head >= q->size)
            q->head = 0;
        q->len++;
    }
}
#endif


#if !defined(CONFIG_FAST_IRQ4) || !defined(CONFIG_FAST_IRQ3)

/*
 * Slower serial interrupt routine, called from _irq_com with passed irq #
 * Reads all FIFO data available per interrupt and can provide serial stats
 */
void rs_irq(int irq, struct pt_regs *regs)
{
    struct serial_info *sp = &ports[(int)irq_to_port[irq]];
    char *io = sp->io;
    struct ch_queue *q = &sp->tty->inq;

    int status = INB(io + UART_LSR);                    /* check for data overrun*/
    if ((status & UART_LSR_DR) == 0)                    /* QEMU may interrupt w/no data*/
        return;

#if UNUSED      // turn on for serial stats
    if (status & UART_LSR_OE)
        printk("serial: data overrun\n");
    if (status & (UART_LSR_FE|UART_LSR_PE))
        printk("serial: frame/parity error\n");
#endif

    /* read uart/fifo until empty*/
    do {
        unsigned char c = INB(io + UART_RX);            /* Read received data */
        if (!tty_intcheck(sp->tty, c))
            chq_addch_nowakeup(q, c);
    } while (INB(io + UART_LSR) & UART_LSR_DR); /* while data available (for FIFOs)*/

    if (q->len)         /* don't wakeup unless chars else EINTR result*/
        wake_up(&q->wait);
}

#endif  // !defined(CONFIG_FAST_IRQ4) || !defined(CONFIG_FAST_IRQ3)


static void rs_release(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];

    debug_tty("SERIAL close %P\n");
    if (--tty->usecount == 0) {
        OUTB(0, port->io + UART_IER);   /* Disable all interrupts */
        free_irq(port->irq);
        tty_freeq(tty);
    }
}

static int rs_open(struct tty *tty)
{
    register struct serial_info *port = &ports[tty->minor - RS_MINOR_OFFSET];
    int err;

    debug_tty("SERIAL open %P\n");

    if (!(port->flags & SERF_EXIST))
        return -ENXIO;

    /* increment use count, don't init if already open*/
    if (tty->usecount++)
        return 0;

    switch(port->irq) {
#ifdef CONFIG_FAST_IRQ4
    case 4:
        err = request_irq(port->irq, (irq_handler) _irq_com1, INT_SPECIFIC);
        break;
#endif
#ifdef CONFIG_FAST_IRQ3
    case 3:
        err = request_irq(port->irq, (irq_handler) _irq_com2, INT_SPECIFIC);
        break;
#endif
    default:
        err = request_irq(port->irq, rs_irq, INT_GENERIC);
        break;
    }
    if (err) goto errout;
    irq_to_port[port->irq] = port - ports;      /* Map irq to this tty # */

    err = tty_allocq(tty, RSINQ_SIZE, RSOUTQ_SIZE);
    if (err) {
errout:
        --tty->usecount;
        return err;
    }

    /* clear RX buffer */
    INB(port->io + UART_LSR);

    /* enable FIFO and flush input*/
#ifdef CONFIG_HW_SERIAL_FIFO
    if ((port->flags & SERF_TYPE) > ST_16550)
        OUTB(UART_FCR_ENABLE_FIFO14, port->io + UART_FCR);
#else
    /* flush input*/
    flush_input(port);
#endif

    INB(port->io + UART_IIR);
    INB(port->io + UART_MSR);

    /* set serial port parameters to match ports[rs_minor] */
    update_port(port);

    /* enable receiver data interrupt*/
    OUTB(UART_IER_RDI, port->io + UART_IER);

    OUTB(port->mcr, port->io + UART_MCR);

    /* clear Line/Modem Status, Intr ID and RX register */
    INB(port->io + UART_LSR);
    INB(port->io + UART_RX);
    INB(port->io + UART_IIR);
    INB(port->io + UART_MSR);

    return 0;
}

#if UNUSED      // removed for bloat
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
    debug("rs_ioctl: sp = %d, cmd = %d\n", tty->minor - RS_MINOR_OFFSET, cmd);
    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        /* verified_memcpy*fs() already called by ntty.c ioctl handler*/
        //FIXME: update_port() only sets baud rate from termios, not parity or wordlen*/
        update_port(port);      /* ignored return value*/
        break;
#if UNUSED
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

static void rs_init(void)
{
    register struct serial_info *sp = ports;
    register struct tty *tty = ttys + NR_CONSOLES;

    do {
        if (!rs_probe(sp)) {
            sp->tty = tty;
            update_port(sp);
        }
        tty++;
    } while (++sp < &ports[NR_SERIAL]);
}

/* note: this function will be called prior to serial_init if serial console set*/
void rs_conout(dev_t dev, int Ch)
{
    register struct serial_info *sp = &ports[MINOR(dev) - RS_MINOR_OFFSET];

    while (!(INB(sp->io + UART_LSR) & UART_LSR_THRE))
        continue;
    outb(Ch, sp->io + UART_TX);
}

#ifdef CONFIG_BOOTOPTS
/* note: this function may be called prior to serial_init if serial console set */
void INITPROC rs_setbaud(dev_t dev, unsigned long baud)
{
    register struct serial_info *sp = &ports[MINOR(dev) - RS_MINOR_OFFSET];
    register struct tty *tty = ttys + NR_CONSOLES + MINOR(dev) - RS_MINOR_OFFSET;
    unsigned int b;     /* use smaller 16-bit width to reduce code*/

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

    sp->tty = tty;      /* force tty association with serial port*/
    tty->termios.c_cflag = b | CS8;
    update_port(sp);
}
#endif

void INITPROC serial_init(void)
{
    register struct serial_info *sp = ports;
    int ttyno = 0;
    static const char *serial_type[] = {
        "n 8250",
        " 16450",
        " 16550",
        " 16550A",
        " 16750",
        " UNKNOWN",
    };

    rs_init();

    do {
        if (sp->tty != NULL) {
            printk("ttyS%d at %x, irq %d is a%s\n", ttyno,
                       sp->io, sp->irq, serial_type[sp->flags & SERF_TYPE]);
        }
        sp++;
    } while (++ttyno < NR_SERIAL);
}

struct tty_ops rs_ops = {
    rs_open,
    rs_release,
    rs_write,
    NULL,
    rs_ioctl,
    rs_conout
};
