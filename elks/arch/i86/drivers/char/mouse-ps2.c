/*
 * Minimal PS/2 mouse driver for ELKS
 *
 * 23 Jan 2026 Original version by @toncho11, adapted by Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/ntty.h>
#include <arch/io.h>
#include <arch/irq.h>

#ifdef CONFIG_MOUSE_PS2

#define IRQ_MOUSE       12      /* mouse interrupt, may conflict with networking */

/* i8042 ports */
#define I8042_DATA      0x60
#define I8042_STATUS    0x64
#define I8042_CMD       0x64

/* status bits */
#define ST_OBF          0x01
#define ST_AUXDATA      0x20

/* Minimal mouse enable (no reset, BIOS-friendly) */
static void ps2_mouse_enable(void)
{
    /* FIXME controller ready (status bit 1) should be read before each write!! */
    /* Enable AUX (mouse) device */
    outb(0xA8, I8042_CMD);

    /* FIXME unclear whether the following enables IRQ 12 */
    /* Enable data reporting */
    outb(0xD4, I8042_CMD);   /* next byte to mouse */
    outb(0xF4, I8042_DATA);  /* enable streaming */

    /* FIXME this could cause OS hang if no PS/2 controller present */
    /* Eat ACK (0xFA) */
    while (!(inb(I8042_STATUS) & ST_OBF))
        ;
    inb(I8042_DATA);
}

/* IRQ handler */
static void ps2_irq(int irq, struct pt_regs *regs)
{
    struct ch_queue *q = &ttys[NR_CONSOLES + NR_PTYS + NR_SERIAL].inq;
    unsigned char c;

    /* Read all available mouse bytes */
    while ((inb(I8042_STATUS) & (ST_OBF | ST_AUXDATA)) == (ST_OBF | ST_AUXDATA)) {
        c = inb(I8042_DATA);
        chq_addch_nowakeup(q, c);
    }

    if (q->len)
        wake_up(&q->wait);
}

static int ps2_open(struct tty *tty)
{
    int err;

    if (tty->usecount)      /* exclusive use only */
        return -EBUSY;

    err = request_irq(IRQ_MOUSE, ps2_irq, INT_GENERIC);
    if (err)
        return err;

    err = tty_allocq(tty, MOUSEINQ_SIZE, 0);  /* input only */
    if (err) {
        free_irq(IRQ_MOUSE);
        return err;
    }

    tty->usecount = 1;
    ps2_mouse_enable();     /* minimal init */

    return 0;
}

static void ps2_release(struct tty *tty)
{
    if (--tty->usecount == 0) {
        /* FIXME needs code to disable i8042 interrupts here!! */
        free_irq(IRQ_MOUSE);
        tty_freeq(tty);
    }
}

static int ps2_write(struct tty *tty)
{
    return 0;   /* mouse is read-only */
}

static int ps2_ioctl(struct tty *tty, int cmd, char *arg)
{
    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        return 0;
    }
    return -EINVAL;
}

struct tty_ops ps2_mouse_ops = {
    ps2_open,
    ps2_release,
    ps2_write,
    NULL,
    ps2_ioctl,
    NULL
};
#endif
