/*
 * NEC V25 Headless Console
 *
 * This file contains code used for the embedded NEC V25 family only
 * Based on Santiago Hormazabal console-serial-8018x.c
 *
 * Uses the MCU internal Serial 1 (and optional Serial 0) port as tty console.
 * 
 * If Serial 0 is used as SPI for SD card interface (CONFIG_HW_SPI is defined),
 * only Serial 1 can be used as console (/dev/tty1). If sofware SPI is 
 * configure, Serial 0 is setup as second console (/dev/tty2), see config.h
 *
 * 23 Oct. 2025 swausd
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>
#include <linuxmt/kernel.h>
#include <arch/irq.h>
#include <arch/necv25.h>
#include "console.h"

struct serial_info {
    const unsigned int  io_base;
    const unsigned int  io_irq_rx;
    const unsigned int  io_irq_tx;
    const unsigned char irq_no_rx;
    const unsigned char irq_no_tx;
    unsigned char count;
    unsigned char n;
    int intrchar; /* used by fast handler for ^C SIGINT processing */
    struct tty *tty;
};


// Baud rate is already initialized to 115200 baud by startup code,
// so first switching to 115200 baud from ELKS is not necessary
// but shows artefacts. This init values prevent first switching

#define INIT_BAUD_COUNT (CONFIG_NECV25_FCPU / (115200UL * 2UL * 2UL))
#define INIT_BAUD_N     0

static struct serial_info ports[] = {
    /* Serial 0 is used as SPI for SD card interface */
    /* Serial 1 */ { NEC_RXB1, NEC_INTSR1, NEC_INTST1, UART1_IRQ_RX, UART1_IRQ_TX, INIT_BAUD_COUNT, INIT_BAUD_N, 0, &ttys[0] },
#ifdef CONFIG_FAST_IRQ2_NECV25
    /* Serial 0 */ { NEC_RXB0, NEC_INTSR0, NEC_INTST0, UART2_IRQ_RX, UART2_IRQ_TX, INIT_BAUD_COUNT, INIT_BAUD_N, 0, &ttys[1] },
#endif
};


/* UART clock baudrate_compares per baud rate */
static const struct baud_tab {
    unsigned char count; /* NEC_BRGx counter              */
    unsigned char n;     /* NEC_SCCx prescaler selector n */
} baud_setup[] = {                                          /*         counter n */
    {0,                                          15},       /*  0 = B0       0 f */
    {(CONFIG_NECV25_FCPU / (    50UL * 2UL * 2048UL)), 10}, /*  1 = B50     72 a */
    {(CONFIG_NECV25_FCPU / (    75UL * 2UL * 2048UL)), 10}, /*  2 = B75     48 a */
    {(CONFIG_NECV25_FCPU / (   110UL * 2UL * 1024UL)),  9}, /*  3 = B110    65 9 */
    {(CONFIG_NECV25_FCPU / (   134UL * 2UL * 1024UL)),  9}, /*  4 = B134    54 9 */
    {(CONFIG_NECV25_FCPU / (   150UL * 2UL * 1024UL)),  9}, /*  5 = B150    48 9 */
    {(CONFIG_NECV25_FCPU / (   200UL * 2UL *  512UL)),  8}, /*  6 = B200    72 8 */
    {(CONFIG_NECV25_FCPU / (   300UL * 2UL *  512UL)),  8}, /*  7 = B300    48 8 */
    {(CONFIG_NECV25_FCPU / (   600UL * 2UL *  256UL)),  7}, /*  8 = B600    48 7 */
    {(CONFIG_NECV25_FCPU / (  1200UL * 2UL *  128UL)),  6}, /*  9 = B1200   48 6 */
    {(CONFIG_NECV25_FCPU / (  1800UL * 2UL *  128UL)),  6}, /* 10 = B1800   32 6 */
    {(CONFIG_NECV25_FCPU / (  2400UL * 2UL *   64UL)),  5}, /* 11 = B2400   48 5 */
    {(CONFIG_NECV25_FCPU / (  4800UL * 2UL *   16UL)),  3}, /* 12 = B4800   96 3 */
    {(CONFIG_NECV25_FCPU / (  9600UL * 2UL *   16UL)),  3}, /* 12 = B4800   96 3 */
    {(CONFIG_NECV25_FCPU / ( 19200UL * 2UL *    8UL)),  2}, /* 14 = B19200  48 2 */
    {(CONFIG_NECV25_FCPU / ( 38400UL * 2UL *    4UL)),  1}, /* 15 = B38400  48 1 */
    {(CONFIG_NECV25_FCPU / ( 57600UL * 2UL *    2UL)),  0}, /* 16 = B57600  64 0 */
//  {(CONFIG_NECV25_FCPU / (115200UL * 2UL *    2UL)),  0}, /* 17 = B115200 32 0 */
    { INIT_BAUD_COUNT,                       INIT_BAUD_N }, /* 17 = B115200 32 0 */ /* preset for inital serial_info */
    {(CONFIG_NECV25_FCPU / (230400UL * 2UL *    2UL)),  0}  /*  0 = B230400 16 0 */
};


/* serial receive interrupt routine, reads and queues received character */
#ifdef CONFIG_FAST_IRQ1_NECV25
extern void asm_fast_irq1_necv25(int irq, struct pt_regs *regs);   /* initial entry point */

void sercon_fast_irq1_necv25(void)
{
    struct serial_info *sp = &ports[0];
    struct ch_queue *q = &sp->tty->inq;
    unsigned char c;

    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../../include/arch/necv25.inc\"                               \n"\
            "push  %%ds                                                                     \n"\
            "movw  $NEC_HW_SEGMENT, %%bx  // load DS to access memmory mapped CPU registers \n"\
            "movw  %%bx, %%ds                                                               \n"\
            "movb  %%ds:(%%Si), %%al      // get char                                       \n"\
            "//testb $0x07, %%ds:SCE(%%Si)  // test for receive errors...                   \n"\
            "//jz    1f                                                                     \n"\
            "//movb  %%ds:SCE(%%Si), %%al   // ...and return error flag if any              \n"\
            "//orb   $0x80, %%al                                                            \n"\
            "//1:                                                                           \n"\
            "pop   %%ds                                                                     \n"\
            : "=Ral"(c)                \
            : "S"   (sp->io_base)      \
            : "bx", "memory" );        \
    });

    if (q->len < q->size) {
        q->base[q->head] = c;
        if (++q->head >= q->size)
            q->head = 0;
        q->len++;
    }   // silently ignore buffer overflow

    if ((c == 03) || (c == 26) || (c == 28))    /* assumes VINTR = ^C, VSUSP = Ẑ, VQUIT = ^\ and byte queued anyways */
        sp->intrchar = c;
}

#else

void irq_rx(int irq, struct pt_regs *regs)
{
    struct serial_info *sp = &ports[0];
    struct ch_queue *q = &sp->tty->inq;
    unsigned char c;

    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../../include/arch/necv25.inc\"                               \n"\
            "push  %%ds                                                                     \n"\
            "movw  $NEC_HW_SEGMENT, %%bx  // load DS to access memmory mapped CPU registers \n"\
            "movw  %%bx, %%ds                                                               \n"\
            "movb  %%ds:(%%Si), %%al      // get char                                       \n"\
            "pop   %%ds                                                                     \n"\
            : "=Ral"(c)                \
            : "S"   (sp->io_base)      \
            : "bx", "memory" );        \
    });

    if (c) {
        if (q->len < q->size) {
            q->base[q->head] = c;
            if (++q->head >= q->size)
                q->head = 0;
            q->len++;
        }
        if ((c == 03) || (c == 26) || (c == 28))    /* assumes VINTR = ^C, VSUSP = Ẑ, VQUIT = ^\ and byte queued anyways */
            sp->intrchar = c;
        mark_bh(SERIAL_BH);
    }
}
#endif

#ifdef CONFIG_FAST_IRQ2_NECV25
extern void asm_fast_irq2_necv25(int irq, struct pt_regs *regs);   /* initial entry point */

void sercon_fast_irq2_necv25(void)
{
    struct serial_info *sp = &ports[1];
    struct ch_queue *q = &sp->tty->inq;
    unsigned char c;

    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../../include/arch/necv25.inc\"                               \n"\
            "push  %%ds                                                                     \n"\
            "movw  $NEC_HW_SEGMENT, %%bx  // load DS to access memmory mapped CPU registers \n"\
            "movw  %%bx, %%ds                                                               \n"\
            "movb  %%ds:(%%Si), %%al      // get char                                       \n"\
            "//testb $0x07, %%ds:SCE(%%Si)  // test for receive errors...                   \n"\
            "//jz    1f                                                                     \n"\
            "//movb  %%ds:SCE(%%Si), %%al   // ...and return error flag if any              \n"\
            "//orb   $0x80, %%al                                                            \n"\
            "//1:                                                                           \n"\
            "pop   %%ds                                                                     \n"\
            : "=Ral"(c)                \
            : "S"   (sp->io_base)      \
            : "bx", "memory" );        \
    });

    if (q->len < q->size) {
        q->base[q->head] = c;
        if (++q->head >= q->size)
            q->head = 0;
        q->len++;
    }   // silently ignore buffer overflow

    if ((c == 03) || (c == 26) || (c == 28))    /* assumes VINTR = ^C, VSUSP = Ẑ, VQUIT = ^\ and byte queued anyways */
        sp->intrchar = c;
}
#endif


/* busy-loop and write a character to the UART */
static void serial_putc(const struct serial_info *sp, byte_t c)
{
    __extension__ ({                   \
        asm volatile (                 \
            ".include \"../../../../include/arch/necv25.inc\"                                           \n"\
            "push  %%ds                                                                                 \n"\
            "movw  $NEC_HW_SEGMENT, %%bx              // load DS to access memmory mapped CPU registers \n"\
            "movw  %%bx, %%ds                                                                           \n"\
            "1:                                                                                         \n"\
            "testb $IRQFLAG, %%ds:STIC(%%si)          // STIC: wait for Tx ready                        \n"\
            "jz    1b                                                                                   \n"\
            "pushf                                    // save iqr status and disable all interrupts     \n"\
            "cli                                                                                        \n"\
            "movb  $(IRQMSK+IRQPRID), %%ds:STIC(%%si) // STIC: clear Tx ready flag                      \n"\
            "movb  %%al, %%ds:TXB(%%si)               // TXB: send char                                 \n"\
            "popf                                     // restore flags and irq status                   \n"\
            "pop   %%ds                                                                                 \n"\
            :                          \
            : "S"   (sp->io_base),     \
              "Ral" (c)                \
            : "bx", "memory" );        \
    });
}


/* update UART with current port termios settings */
static void update_port(struct serial_info *port)
{
    unsigned int cflags;
    unsigned char count;
    unsigned char n;

    cflags = port->tty->termios.c_cflag & CBAUD;

    if (cflags & CBAUDEX)
        cflags = B38400 + (cflags & 03);

    /* get which baud rate compare value is requested */
    count = baud_setup[cflags].count;
    n     = baud_setup[cflags].n;

    /* FIXME: should update lcr parity and data width from termios values*/

    /* update baudrate compare only if changed */
    if ((count != port->count) || (n != port->n))
    {
        port->count = count;
        port->n     = n;

        /* Set the baudrate */
        __extension__ ({                   \
            asm volatile (                 \
                ".include \"../../../../include/arch/necv25.inc\"                                                             \n"\
                "push   %%ds                                                                                                  \n"\
                "movw   $NEC_HW_SEGMENT, %%bx                     // load DS to access memmory mapped CPU registers           \n"\
                "movw   %%bx, %%ds                                                                                            \n"\
                "pushf                                            // save iqr status and disable all interrupts               \n"\
                "cli                                                                                                          \n"\
                "movb   $(NOPRTY+CL8+SL1), %%ds:SCM(%%si)         // SCM: Rx/Tx disable, 8 data, 1 stop bit, no parity, async \n"\
                "movb   %%al, %%ds:SCC(%%si)                      // SCC: set prescaler                                       \n"\
                "movb   %%ah, %%ds:BRG(%%si)                      // BRG: set counter                                         \n"\
                "movb   $(TXE+RXE+NOPRTY+CL8+SL1), %%ds:SCM(%%si) // SCM: Rx/Tx enable, 8 data, 1 stop bit, no parity, async  \n"\
                "popf                                             // restore flags and irq status                             \n"\
                "pop    %%ds                                                                                                  \n"\
                :                          \
                : "S"   (port->io_base),   \
                  "Ral" (n),               \
                  "Rah" (count)            \
                : "bx", "memory" );        \
        });
    }
}

/* Called from main.c */
void INITPROC console_init(void)
{
    struct serial_info *sp = ports;;

    do
    {
        /*
         * The serial port might have been not be configured until this point,
         * so set it up just in case.
         */

        /* Asynchronous, 8 data bits, 1 start, 1 stop, no parity */
        __extension__ ({                   \
             asm volatile (                 \
             ".include \"../../../../include/arch/necv25.inc\"                                                             \n"\
             "push   %%ds                                                                                                  \n"\
             "movw   $NEC_HW_SEGMENT, %%bx                     // load DS to access memmory mapped CPU registers           \n"\
             "movw   %%bx, %%ds                                                                                            \n"\
             "movb   $(TXE+RXE+NOPRTY+CL8+SL1), %%ds:SCM(%%si) // SCM: Rx/Tx enable, 8 data, 1 stop bit, no parity, async  \n"\
             "pop    %%ds                                                                                                  \n"\
             :                          \
             : "S"   (sp->io_base)      \
             : "bx", "memory" );        \
        });

        if (sp == &ports[0])
#ifdef CONFIG_FAST_IRQ1_NECV25
            request_irq(sp->irq_no_rx, asm_fast_irq1_necv25, INT_SPECIFIC);   // for RX
#else
            request_irq(sp->irq_no_rx, irq_rx, INT_GENERIC);                  // for RX
#endif
#ifdef CONFIG_FAST_IRQ2_NECV25
        else
            request_irq(sp->irq_no_rx, asm_fast_irq2_necv25, INT_SPECIFIC);   // for RX
#endif
        /* Set the baudrate, and enable the UART receiver */
        update_port(sp);
    }
    while (++sp < &ports[NR_CONSOLES]);
}

void bell(void)
{
    serial_putc(&ports[0], 7);   /* send ^G to the Serial 0 */
}

static void sercon_conout(dev_t dev, int Ch)
{
    struct serial_info *sp = &ports[MINOR(dev) - TTY_MINOR_OFFSET];

    if (Ch == '\n') {
        serial_putc(sp, '\r');
    }
    serial_putc(sp, Ch);
}

static int sercon_ioctl(struct tty *tty, int cmd, char *arg)
{
    struct serial_info *port = &ports[tty->minor - TTY_MINOR_OFFSET];

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

static int sercon_write(struct tty *tty)
{
    struct serial_info *port = &ports[tty->minor - TTY_MINOR_OFFSET];
    int cnt = 0;

    while (tty->outq.len > 0) {
        serial_putc(port, (byte_t)tty_outproc(tty));
        cnt++;
    }
    return cnt;
}

static void sercon_release(struct tty *tty)
{
    if (--tty->usecount == 0)
        tty_freeq(tty);
}

static int sercon_open(struct tty *tty)
{
    struct serial_info *port = &ports[tty->minor - TTY_MINOR_OFFSET];

    /* increment use count, don't init if already open*/
    if (tty->usecount++)
        return 0;

    port->intrchar = 0;
    init_bh(SERIAL_BH, serial_bh);
    return tty_allocq(tty, RSINQ_SIZE, RSOUTQ_SIZE);
}

/* check for SIGINT and wakeup waiting processes */
static void pump_port(struct serial_info *sp)
{
    struct tty *ttyp = sp->tty;
    struct ch_queue *q = &ttyp->inq;

    if (ttyp->usecount && q->len) {
        if (sp->intrchar) {
            tty_intcheck(ttyp, sp->intrchar);
            sp->intrchar = 0;
        }
        if (q->len == 1) {
            /* attempt process (again) of VINTR, also VQUIT and ^P-^N debug chars */
            if (tty_intcheck(ttyp, chq_peekch(q))) {
                (void)chq_getch(q);     /* discard received character */
                return;
            }
        }
        wake_up(&q->wait);
    }
}

/* serial interrupt bottom half - check ring buffer and wakeup waiting processes */
void serial_bh(void)
{
#if defined(CONFIG_FAST_IRQ1_NECV25)
    pump_port(&ports[0]);
#endif
#if defined(CONFIG_FAST_IRQ2_NECV25)
    pump_port(&ports[1]);
#endif
}

struct tty_ops necv25con_ops = {
    sercon_open,
    sercon_release,
    sercon_write,
    NULL,
    sercon_ioctl,
    sercon_conout
};
