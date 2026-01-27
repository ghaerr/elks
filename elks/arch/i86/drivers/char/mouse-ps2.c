/*
 * Minimal PS/2 mouse driver for ELKS
 *
 * 23 Jan 2026 Original version by Anton Andreev, adapted by Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/ntty.h>
#include <linuxmt/kernel.h>
#include <arch/io.h>
#include <arch/irq.h>

#ifdef CONFIG_MOUSE_PS2

#define IRQ_MOUSE       12          /* mouse interrupt, may conflict with networking */
#define MAX_RETRIES     60

/* 8042 ports */
#define DATA            0x60
#define STATUS          0x64
#define COMMAND         0x64

/* 8042 commands using outb() */
#define WRITE_CTRLR     0x60        /* send command to controller */
#define WRITE_MOUSE     0xd4        /* send command to aux device (mouse) */
#define DISABLE_AUX     0xa7        /* disable aux (mouse) port */
#define ENABLE_AUX      0xa8        /* enable aux (mouse) port */

/* commands using WRITE_CTRLR/write_controller() */
#define ENABLE_INTS     0x47        /* enable interrupts */
#define DISABLE_INTS    0x65        /* disable interrupts */

/* comands using WRITE_MOUSE/write_mouse() */
#define ENABLE_AUX_DEV  0xf4        /* enable aux device */
#define DISABLE_AUX_DEV 0xf5        /* disable aux device */

/* controller status bits */
#define IBUF_FULL       0x02        /* input buffer to device full */
#define OBF             0x01        /* output buffer full */
#define AUXDATA         0x20        /* mouse data */

static void poll_aux_status(void)
{
    int retries = MAX_RETRIES;

    while (retries--) {
        unsigned char st = inb_p(STATUS);

        /* Drain any pending output byte (kbd or mouse) */
        if (st & OBF) {
			(void)inb_p(DATA);
			continue;
		}

        /* Input buffer clear and no output pending -> ready */
        if (!(st & IBUF_FULL))
            return;
    }
}

static unsigned char read_ccb(void)
{
    poll_aux_status();
    outb_p(0x20, COMMAND);          /* read command byte */
    poll_aux_status();
    return inb_p(DATA);
}

static void write_ccb(unsigned char ccb)
{
    poll_aux_status();
    outb_p(WRITE_CTRLR, COMMAND);   /* 0x60 */
    poll_aux_status();
    outb_p(ccb, DATA);
}

/* write command to mouse */
static void write_mouse(int cmd)
{
    poll_aux_status();
    outb_p(WRITE_MOUSE, COMMAND);       /* send command to mouse (aux device) */
    poll_aux_status();
    outb_p(cmd, DATA);                  /* command */
}

/* write command to controller */
static void write_controller(int cmd)
{
    poll_aux_status();
    outb_p(WRITE_CTRLR, COMMAND);       /* send command to controller */
    poll_aux_status();
    outb_p(cmd, DATA);                  /* command */
}

static void ps2_mouse_disable(void)
{
    unsigned char ccb;

    write_mouse(DISABLE_AUX_DEV);       /* stop mouse packets */

    ccb = read_ccb();
    ccb &= (unsigned char)~0x02;        /* clear IRQ12 enable ONLY */
    write_ccb(ccb);

    poll_aux_status();
    outb_p(DISABLE_AUX, COMMAND);
    poll_aux_status();
}

static void ps2_mouse_enable(void)
{
    unsigned char ccb;

    poll_aux_status();
    outb_p(ENABLE_AUX, COMMAND);

    write_mouse(ENABLE_AUX_DEV);

    ccb = read_ccb();
    ccb |= 0x02;                        /* set IRQ12 enable ONLY */
    write_ccb(ccb);

    poll_aux_status();
}

/* IRQ handler */
static void ps2_irq(int irq, struct pt_regs *regs)
{
    struct ch_queue *q = &ttys[NR_CONSOLES + NR_PTYS + NR_SERIAL].inq;
    unsigned char c;

    /* Read all available mouse bytes */
    while ((inb(STATUS) & (OBF | AUXDATA)) == (OBF | AUXDATA)) {
		c = inb(DATA);
		chq_addch_nowakeup(q, c);
	}

    if (q->len)
        wake_up(&q->wait);
}

static int ps2_open(struct tty *tty)
{
    int err, sav_caps;

    if (tty->usecount)          /* exclusive use only */
        return -EBUSY;

    sav_caps = sys_caps;
    sys_caps |= CAP_IRQ8TO15;   /* force-allow IRQ 12 to allow 8086 emulators to work */
    err = request_irq(IRQ_MOUSE, ps2_irq, INT_GENERIC);
    sys_caps = sav_caps;
    if (err) {
        printk("mouse: irq %d not available, error %d\n", IRQ_MOUSE, -err);
        return err;
    }
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
