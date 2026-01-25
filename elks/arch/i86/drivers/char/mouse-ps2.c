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

#define IRQ_MOUSE       12          /* mouse interrupt, may conflict with networking */
#define MAX_RETRIES     60

/* 8042 ports */
#define DATA            0x60
#define STATUS          0x64
#define COMMAND         0x64

/* 8042 commands */
#define DISABLE_INTS    0x65        /* disable interrupts */
#define ENABLE_INTS     0x47        /* enable interrupts */
#define DISABLE_AUX     0xa7        /* disable aux */
#define ENABLE_AUX      0xa8        /* enable aux */
#define DISABLE_AUX_DEV 0xf5        /* disable aux device */
#define ENABLE_AUX_DEV  0xf4        /* enable aux device */
#define MAGIC_WRITE     0xd4        /* value to send aux device data */
#define CMD_WRITE       0x60        /* value to write to controller */

/* aux controller status bits */
#define IBUF_FULL       0x02        /* input buffer (to device) full */
#define OBUF_FULL       0x21        /* output buffer (from device) full */

static void poll_aux_status(void)
{
    int retries = 0;

    while ((inb(STATUS) & 0x03) && retries < MAX_RETRIES) {
        if ((inb_p(STATUS) & OBUF_FULL) == OBUF_FULL)
            inb_p(DATA);
        retries++;
    }
}

/* write to aux device */
static void aux_write_dev(int val)
{
    poll_aux_status();
    outb_p(MAGIC_WRITE, COMMAND);       /* next byte to mouse */
    poll_aux_status();
    outb_p(val, DATA);                  /* write data */
}

/* write aux device command */
static void aux_write_cmd(int val)
{
    poll_aux_status();
    outb_p(CMD_WRITE, COMMAND);         /* next byte to controller */
    poll_aux_status();
    outb_p(val, DATA);                  /* write data */
}

static void ps2_mouse_disable(void)
{
    aux_write_cmd(DISABLE_INTS);        /* disable controller interrupts */
    poll_aux_status();
    outb_p(DISABLE_AUX, COMMAND);       /* disable aux device */
    poll_aux_status();
}

static void ps2_mouse_enable(void)
{
    poll_aux_status();
    outb_p(ENABLE_AUX, COMMAND);        /* enable aux */
    aux_write_dev(ENABLE_AUX_DEV);      /* enable aux device streaming */
    aux_write_cmd(ENABLE_INTS);         /* enable controller interrupts */
    poll_aux_status();
}

/* IRQ handler */
static void ps2_irq(int irq, struct pt_regs *regs)
{
    struct ch_queue *q = &ttys[NR_CONSOLES + NR_PTYS + NR_SERIAL].inq;
    unsigned char c;

    /* Read all available mouse bytes */
    while ((inb(STATUS) & OBUF_FULL) == OBUF_FULL) {
        c = inb(DATA);
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
    ps2_mouse_enable();

    return 0;
}

static void ps2_release(struct tty *tty)
{
    if (--tty->usecount == 0) {
        ps2_mouse_disable();
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
