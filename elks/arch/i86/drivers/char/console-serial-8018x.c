/*
 * 8081X Headless Console
 *
 * This file contains code used for the embedded 8018X family only.
 * 
 * Uses the internal Serial 0 port as tty console.
 * 
 * 07 Nov 21 Santiago Hormazabal
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/8018x.h>
#include "console.h"

struct serial_info {
    const unsigned int  io_cmp;
    const unsigned int  io_ccon;
    const unsigned int  io_sts;
    const unsigned int  io_rxbuf;
    const unsigned int  io_txbuf;
    const unsigned char irq_rx;
    const unsigned char irq_tx;

    /**
     * The baud rate generator is composed of a 15-bit counter 
     * register (BxCNT) and a 15-bit compare register (BxCMP). BxCNT
     * is a free-running counter that is incremented by the baud
     * timebase clock. The baud timebase clock can be either the
     * internal CPU clock or an external clock applied to the BCLK pin.
     * For the Asynchronous Mode 1, BxCMP can be calculated as follows
     * BxCMP = (Fcpu / (Baudrate * 8)) - 1.
     * Note: This driver uses the internal CPU clock.
     */
    unsigned int  baudrate_compare;

    struct tty *tty;
};

static struct serial_info ports[1] = {
    /* Serial 0 */ { PCB_B0CMP, PCB_S0CON, PCB_S0STS, PCB_R0BUF, PCB_T0BUF, UART0_IRQ_RX, UART0_IRQ_TX, 0, &ttys[0] },
    /* TODO: Serial 1 not available { PCB_B1CMP, PCB_S1CON, PCB_S1STS, PCB_R1BUF, PCB_T1BUF, 0, 0, 0, NULL} */
};

/* UART clock baudrate_compares per baud rate */
static const unsigned int baudrate_compares[] = {
    0,  /*  0 = B0      */
    (CONFIG_8018X_FCPU * 1000000UL / (50UL * 8UL)) - 1,     /*  1 = B50     */
    (CONFIG_8018X_FCPU * 1000000UL / (75UL * 8UL)) - 1,     /*  2 = B75     */
    (CONFIG_8018X_FCPU * 1000000UL / (110UL * 8UL)) - 1,    /*  3 = B110    */
    (CONFIG_8018X_FCPU * 1000000UL / (134UL * 8UL)) - 1,    /*  4 = B134    */
    (CONFIG_8018X_FCPU * 1000000UL / (150UL * 8UL)) - 1,    /*  5 = B150    */
    (CONFIG_8018X_FCPU * 1000000UL / (200UL * 8UL)) - 1,    /*  6 = B200    */
    (CONFIG_8018X_FCPU * 1000000UL / (300UL * 8UL)) - 1,    /*  7 = B300    */
    (CONFIG_8018X_FCPU * 1000000UL / (600UL * 8UL)) - 1,    /*  8 = B600    */
    (CONFIG_8018X_FCPU * 1000000UL / (1200UL * 8UL)) - 1,   /*  9 = B1200   */
    (CONFIG_8018X_FCPU * 1000000UL / (1800UL * 8UL)) - 1,   /* 10 = B1800   */
    (CONFIG_8018X_FCPU * 1000000UL / (2400UL * 8UL)) - 1,   /* 11 = B2400   */
    (CONFIG_8018X_FCPU * 1000000UL / (4800UL * 8UL)) - 1,   /* 12 = B4800   */
    (CONFIG_8018X_FCPU * 1000000UL / (9600UL * 8UL)) - 1,   /* 13 = B9600   */
    (CONFIG_8018X_FCPU * 1000000UL / (19200UL * 8UL)) - 1,  /* 14 = B19200  */
    (CONFIG_8018X_FCPU * 1000000UL / (38400UL * 8UL)) - 1,  /* 15 = B38400  */
    (CONFIG_8018X_FCPU * 1000000UL / (57600UL * 8UL)) - 1,  /* 16 = B57600  */
    (CONFIG_8018X_FCPU * 1000000UL / (115200UL * 8UL)) - 1, /* 17 = B115200 */
    (CONFIG_8018X_FCPU * 1000000UL / (230400UL * 8UL)) - 1, /*  0 = B230400 */
};

/* serial receive interrupt routine, reads and queues received character */
void irq_rx(int irq, struct pt_regs *regs)
{
    struct serial_info *sp = &ports[0];

    /* Read UART status */
    unsigned int status = inw(sp->io_sts);

    if (status & 0x94) {
        /* discard parity, framing and overrun errors */
        return;
    }

    /* Read received data */
    unsigned char c = inb(sp->io_rxbuf);

    if (!tty_intcheck(sp->tty, c)) {
        chq_addch(&sp->tty->inq, c);
    }
}

/* serial transmit interrupt routine, doesn't do anything */
void irq_tx(int irq, struct pt_regs *regs)
{
}

/* busy-loop and write a character to the UART */
static void serial_putc(const struct serial_info *sp, byte_t c)
{
    /* Test for TXE bit set on the status register */
    while((inw(sp->io_sts) & 0x8) == 0);
    /* Write the character */
    outb(c, sp->io_txbuf);
}

/* update UART with current port termios settings */
static void update_port(struct serial_info *port)
{
    unsigned int cflags;
    unsigned int baudrate_compare;

    cflags = port->tty->termios.c_cflag & CBAUD;
    if (cflags & CBAUDEX) {
        cflags = B38400 + (cflags & 03);
    }
    /* get which baud rate compare value is requested */
    baudrate_compare = baudrate_compares[cflags];

    clr_irq();

    /* Disable receiver */
    outw(inw(port->io_ccon) & ~0x0020, port->io_ccon);

    /* update baudrate compare only if changed, since we have not TCSETW */
    if (baudrate_compare != port->baudrate_compare) {
        port->baudrate_compare = baudrate_compare;

        /* Set the baudrate compare value, using the internal clock mask */
        outw(baudrate_compare | 0x8000, port->io_cmp);
    }

    /* TODO: add hardware handshake options? */

    /* Enable receiver */
    outw(inw(port->io_ccon) | 0x0020, port->io_ccon);

    set_irq();
}

/* Called from main.c! */
void INITPROC console_init(void)
{
    struct serial_info *sp = &ports[0]; /* TODO: add support for Serial 1 */

    /*
     * The serial port might have been not be configured until this point,
     * so set it up just in case.
     */
    
    /* 0x0001 = Mode 1, Asynchronous, 8 data bits, 1 start, 1 stop, no parity, no handshake */
    outw(0x0001, sp->io_ccon);

    /* setup the UART 0 (the only one that has IRQs) */
    request_irq(sp->irq_rx, irq_rx, INT_GENERIC);
    request_irq(sp->irq_tx, irq_tx, INT_GENERIC);

    /* Set the baudrate, and enable the UART receiver */
    update_port(sp);

    printk("console_init: 8018X UART\n");
}

void bell(void)
{
    serial_putc(&ports[0], 7);	/* send ^G to the Serial 0 */
}

static void sercon_conout(dev_t dev, int Ch)
{
    struct serial_info *sp = &ports[0]; /* TODO: add support for Serial 1 */

    if (Ch == '\n') {
        serial_putc(sp, '\r');
    }
    serial_putc(sp, Ch);
}

static int sercon_ioctl(struct tty *tty, int cmd, char *arg)
{
    struct serial_info *port = &ports[0]; /* TODO: add support for Serial 1 */

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

static int sercon_write(register struct tty *tty)
{
    struct serial_info *port = &ports[0]; /* TODO: add support for Serial 1 */
    int cnt = 0;

    while (tty->outq.len > 0) {
        serial_putc(port, (byte_t)tty_outproc(tty));
        cnt++;
    }
    return cnt;
}

static void sercon_release(struct tty *tty)
{
    ttystd_release(tty);
}

static int sercon_open(register struct tty *tty)
{
    return ttystd_open(tty);
}

struct tty_ops i8018xcon_ops = {
    sercon_open,
    sercon_release,
    sercon_write,
    NULL,
    sercon_ioctl,
    sercon_conout
};
