/*
 * ELKS IBM PC speaker low-rate event sequencer.
 *
 * This deliberately keeps PIT timer 0 kernel-owned.  PIT channel 2 is used
 * only as a square-wave carrier.  The sequencer uses the normal kernel
 * timer_list path, but schedules only note/rest boundary callbacks rather
 * than running a 100 Hz countdown while active.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/kd.h>
#include <linuxmt/mm.h>
#include <linuxmt/pcspk.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/param.h>
#include <arch/ports.h>

#ifdef CONFIG_ARCH_IBMPC

#define PCSPK_QSIZE        32
#define PCSPK_GATE_BITS    0x03
#define PCSPK_TIMER2_MODE3 0xB6
#define PCSPK_MAX_TICKS    (HZ * 10)    /* per-event stuck-tone guard */

static unsigned int pcspk_q_div[PCSPK_QSIZE];
static unsigned int pcspk_q_ticks[PCSPK_QSIZE];
static unsigned char pcspk_q_flags[PCSPK_QSIZE];
static unsigned char pcspk_head;
static unsigned char pcspk_tail;
static unsigned char pcspk_count;

static void pcspk_timer_fn(int unused);

static struct timer_list pcspk_timer = { NULL, 0, 0, pcspk_timer_fn };
static unsigned char pcspk_timer_on;
static unsigned char pcspk_active;
static unsigned char pcspk_gate_on;
static unsigned int pcspk_hw_divisor;
static pid_t pcspk_owner;

static void pcspk_hw_gate(unsigned char on)
{
    unsigned char v;

    if (on == pcspk_gate_on)
        return;

    v = inb(SPEAKER_PORT);
    if (on)
        v |= PCSPK_GATE_BITS;
    else
        v &= ~PCSPK_GATE_BITS;
    outb(v, SPEAKER_PORT);
    pcspk_gate_on = on;
}

static void pcspk_pit2_load(unsigned int divisor)
{
    outb(PCSPK_TIMER2_MODE3, TIMER_CMDS_PORT);

#ifdef __ia16__
    /*
     * Avoid compiler-generated ``shr ax,cl`` for divisor >> 8 on 8086.
     * AL already holds the low byte and AH the high byte; swap them between
     * the two PIT data writes.
     */
    asm volatile ("outb %%al,%%dx\n\t"
                  "xchg %%al,%%ah\n\t"
                  "outb %%al,%%dx"
        : "+a" (divisor)
        : "d" (TIMER2_PORT));
#else
    outb((unsigned char)divisor, TIMER2_PORT);
    outb((unsigned char)(divisor >> 8), TIMER2_PORT);
#endif
}

static void pcspk_hw_tone(unsigned int divisor)
{
    if (divisor != pcspk_hw_divisor) {
        pcspk_pit2_load(divisor);
        pcspk_hw_divisor = divisor;
    }
    pcspk_hw_gate(1);
}

static void pcspk_cancel_timer(void)
{
    if (pcspk_timer_on) {
        del_timer(&pcspk_timer);
        pcspk_timer_on = 0;
    }
}

static void pcspk_arm_timer(unsigned int ticks)
{
    if (!ticks)
        ticks = 1;
    if (ticks > PCSPK_MAX_TICKS)
        ticks = PCSPK_MAX_TICKS;

    if (!pcspk_timer_on) {
        pcspk_timer.tl_expires = jiffies + ticks;
        add_timer(&pcspk_timer);
        pcspk_timer_on = 1;
    }
}

static void pcspk_stop_locked(void)
{
    pcspk_cancel_timer();
    pcspk_head = pcspk_tail = pcspk_count = 0;
    pcspk_active = 0;
    pcspk_owner = 0;
    pcspk_hw_gate(0);
    pcspk_hw_divisor = 0;
}

void pcspk_seq_stop(void)
{
    flag_t flags;

    save_flags(flags);
    clr_irq();
    pcspk_stop_locked();
    restore_flags(flags);
}

void pcspk_seq_exit(pid_t pid)
{
    flag_t flags;

    save_flags(flags);
    clr_irq();
    if (pcspk_owner == pid)
        pcspk_stop_locked();
    restore_flags(flags);
}

static int pcspk_queue_event(struct pcspk_event *ev)
{
    unsigned char head;
    unsigned int ticks;

    if (pcspk_count >= PCSPK_QSIZE)
        return -EAGAIN;

    ticks = ev->ticks;
    if (!ticks)
        ticks = 1;
    else if (ticks > PCSPK_MAX_TICKS)
        ticks = PCSPK_MAX_TICKS;

    head = pcspk_head;
    pcspk_q_div[head] = ev->divisor;
    pcspk_q_ticks[head] = ticks;
    pcspk_q_flags[head] = ev->flags;
    pcspk_head = (head + 1) & (PCSPK_QSIZE - 1);
    pcspk_count++;
    return 0;
}

static void pcspk_load_next_locked(void)
{
    unsigned char tail;
    unsigned char flags;
    unsigned int divisor;
    unsigned int ticks;

    if (!pcspk_count) {
        pcspk_stop_locked();
        return;
    }

    tail = pcspk_tail;
    divisor = pcspk_q_div[tail];
    ticks = pcspk_q_ticks[tail];
    flags = pcspk_q_flags[tail];
    pcspk_tail = (tail + 1) & (PCSPK_QSIZE - 1);
    pcspk_count--;

    if (flags & PCSPK_F_STOP) {
        pcspk_stop_locked();
        return;
    }

    if ((flags & PCSPK_F_TONE) && divisor)
        pcspk_hw_tone(divisor);
    else
        pcspk_hw_gate(0);

    pcspk_arm_timer(ticks);
}

static void pcspk_start_locked(void)
{
    if (!pcspk_active) {
        pcspk_active = 1;
        pcspk_owner = current->pid;
        pcspk_load_next_locked();
    }
}

static void pcspk_timer_fn(int unused)
{
    flag_t flags;

    unused = unused;

    save_flags(flags);
    clr_irq();
    pcspk_timer_on = 0;

    if (pcspk_active)
        pcspk_load_next_locked();

    restore_flags(flags);
}

int pcspk_seq_ioctl(char *arg)
{
    struct pcspk_seq req;
    struct pcspk_event ev;
    struct pcspk_event *up;
    unsigned int n;
    unsigned int queued = 0;
    int err;
    flag_t flags;

    err = verified_memcpy_fromfs(&req, arg, sizeof(req));
    if (err)
        return err;

    if (req.rate_hz && req.rate_hz != HZ)
        return -EINVAL;

    save_flags(flags);
    clr_irq();

    if (req.flags & (PCSPK_SEQ_F_FLUSH | PCSPK_SEQ_F_STOP))
        pcspk_stop_locked();

    if (req.flags & PCSPK_SEQ_F_STOP) {
        restore_flags(flags);
        return 0;
    }

    if (pcspk_active && pcspk_owner != current->pid) {
        restore_flags(flags);
        return -EBUSY;
    }

    restore_flags(flags);

    up = req.events;
    for (n = 0; n < req.count; n++) {
        err = verified_memcpy_fromfs(&ev, up + n, sizeof(ev));
        if (err)
            return queued ? (int)queued : err;

        save_flags(flags);
        clr_irq();
        if (pcspk_queue_event(&ev)) {
            if (queued)
                pcspk_start_locked();
            restore_flags(flags);
            return queued ? (int)queued : -EAGAIN;
        }
        queued++;
        restore_flags(flags);
    }

    save_flags(flags);
    clr_irq();
    if (queued)
        pcspk_start_locked();
    restore_flags(flags);

    return (int)queued;
}

#endif /* CONFIG_ARCH_IBMPC */
