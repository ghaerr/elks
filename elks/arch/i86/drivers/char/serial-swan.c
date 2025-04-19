/*
 * WonderSwan ELKS Serial Driver
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
#include <arch/ports.h>
#include <arch/swan.h>

static struct tty *tty;
extern struct tty ttys[];

/* serial write - busy loops until transmit buffer available */
static int rs_write(struct tty *tty)
{
    int i = 0;

    while (tty->outq.len > 0) {
	/* Wait until transmitter hold buffer empty */
        while (!(inb(UART_CONTROL_PORT) & UART_TX_READY));
	outb((char)tty_outproc(tty), UART_DATA_PORT);
	i++;
    }
    return i;
}

/* serial interrupt routine, reads and queues received character */
void rs_irq(int irq, struct pt_regs *regs)
{
    struct ch_queue *q = &tty->inq;

    unsigned char c = inb(UART_DATA_PORT); /* Read received data */
    ack_irq(3);
    outb(UART_CONTROL_PORT, UART_OVR_RESET); /* Reset overrun, if any */
    if (!tty_intcheck(tty, c))
	chq_addch(q, c);
}


/* serial close */
static void rs_release(struct tty *tty)
{
    debug_tty("SERIAL close %P\n");
    if (--tty->usecount == 0) {
        outb(0x00, UART_CONTROL_PORT);
	tty_freeq(tty);
    }
}

/* update UART with current port termios settings */
static void update_port(unsigned char restart)
{
    unsigned char ser_baud;
    unsigned char ser_status;

    /* simplification: we only support 38400 Hz and 9600 Hz */
    ser_baud = (tty->termios.c_cflag & CBAUDEX) ? UART_B38400 : UART_B9600;

    ser_status = inb(UART_CONTROL_PORT);
    if (restart || (ser_status & UART_B38400) != ser_baud) {
        /* restart UART with new baud rate */
        outb(UART_OVR_RESET, UART_CONTROL_PORT);
        outb((restart ? UART_ENABLE : (ser_status & UART_ENABLE)) | ser_baud, UART_CONTROL_PORT);
    }
}

/* serial open */
static int rs_open(struct tty *tty)
{
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

    /* restart UART */
    update_port(1);

    return 0;
}

/* note: this function will be called prior to serial_init if serial console set*/
void rs_conout(dev_t dev, int c)
{
    while (!(inb(UART_CONTROL_PORT) & UART_TX_READY));
    outb(c, UART_DATA_PORT);
}

void INITPROC serial_init(void)
{
    tty = ttys + NR_CONSOLES;

    if (request_irq(COM1_IRQ, rs_irq, INT_GENERIC))
       printk("Can't get serial IRQ %d\n", COM1_IRQ);
    else {
       printk("ttyS0 at %x, irq %d\n", UART_DATA_PORT, COM1_IRQ);
       update_port(0);
    }
}

static int rs_ioctl(struct tty *tty, int cmd, char *arg)
{
    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
	update_port(0);
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
    tty = ttys + NR_CONSOLES + MINOR(dev) - RS_MINOR_OFFSET;

    if ((unsigned)baud == 38400) tty->termios.c_cflag = B38400 | CS8;
    else if ((unsigned)baud == 9600) tty->termios.c_cflag = B9600 | CS8;
    else return;

    update_port(0);
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
