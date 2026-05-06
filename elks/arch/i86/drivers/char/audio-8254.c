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
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/param.h>
#include <arch/ports.h>

#define AUDIO_QSIZE        32
#define AUDIO_GATE_BITS    0x03
#define AUDIO_TIMER2_MODE3 0xB6
#define AUDIO_MAX_TICKS    (HZ * 10)    /* per-event stuck-tone guard */

static unsigned int audio_q_div[AUDIO_QSIZE];
static unsigned int audio_q_ticks[AUDIO_QSIZE];
static unsigned char audio_q_flags[AUDIO_QSIZE];
static unsigned char audio_head;
static unsigned char audio_tail;
static unsigned char audio_count;

static void audio_timer_fn(int unused);

static struct timer_list audio_timer = { NULL, 0, 0, audio_timer_fn };
static unsigned char audio_timer_on;
static unsigned char audio_active;
static unsigned char audio_gate_on;
static unsigned int audio_hw_divisor;
static pid_t audio_owner;

static void audio_hw_gate(unsigned char on)
{
    unsigned char v;

    if (on == audio_gate_on)
        return;

    v = inb(SPEAKER_PORT);
    if (on)
        v |= AUDIO_GATE_BITS;
    else
        v &= ~AUDIO_GATE_BITS;
    outb(v, SPEAKER_PORT);
    audio_gate_on = on;
}

static void audio_pit2_load(unsigned int divisor)
{
    outb(AUDIO_TIMER2_MODE3, TIMER_CMDS_PORT);

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

static void audio_hw_tone(unsigned int divisor)
{
    if (divisor != audio_hw_divisor) {
        audio_pit2_load(divisor);
        audio_hw_divisor = divisor;
    }
    audio_hw_gate(1);
}

static void audio_cancel_timer(void)
{
    if (audio_timer_on) {
        del_timer(&audio_timer);
        audio_timer_on = 0;
    }
}

static void audio_arm_timer(unsigned int ticks)
{
    if (!ticks)
        ticks = 1;
    if (ticks > AUDIO_MAX_TICKS)
        ticks = AUDIO_MAX_TICKS;

    if (!audio_timer_on) {
        audio_timer.tl_expires = jiffies + ticks;
        add_timer(&audio_timer);
        audio_timer_on = 1;
    }
}

static void audio_stop_locked(void)
{
    audio_cancel_timer();
    audio_head = audio_tail = audio_count = 0;
    audio_active = 0;
    audio_owner = 0;
    audio_hw_gate(0);
    audio_hw_divisor = 0;
}

void audio_seq_stop(void)
{
    flag_t flags;

    save_flags(flags);
    clr_irq();
    audio_stop_locked();
    restore_flags(flags);
}

void audio_seq_exit(pid_t pid)
{
    flag_t flags;

    save_flags(flags);
    clr_irq();
    if (audio_owner == pid)
        audio_stop_locked();
    restore_flags(flags);
}

static int audio_queue_event(struct audio_event *ev)
{
    unsigned char head;
    unsigned int ticks;

    if (audio_count >= AUDIO_QSIZE)
        return -EAGAIN;

    ticks = ev->ticks;
    if (!ticks)
        ticks = 1;
    else if (ticks > AUDIO_MAX_TICKS)
        ticks = AUDIO_MAX_TICKS;

    head = audio_head;
    audio_q_div[head] = ev->divisor;
    audio_q_ticks[head] = ticks;
    audio_q_flags[head] = ev->flags;
    audio_head = (head + 1) & (AUDIO_QSIZE - 1);
    audio_count++;
    return 0;
}

static void audio_load_next_locked(void)
{
    unsigned char tail;
    unsigned char flags;
    unsigned int divisor;
    unsigned int ticks;

    if (!audio_count) {
        audio_stop_locked();
        return;
    }

    tail = audio_tail;
    divisor = audio_q_div[tail];
    ticks = audio_q_ticks[tail];
    flags = audio_q_flags[tail];
    audio_tail = (tail + 1) & (AUDIO_QSIZE - 1);
    audio_count--;

    if (flags & AUDIO_F_STOP) {
        audio_stop_locked();
        return;
    }

    if ((flags & AUDIO_F_TONE) && divisor)
        audio_hw_tone(divisor);
    else
        audio_hw_gate(0);

    audio_arm_timer(ticks);
}

static void audio_start_locked(void)
{
    if (!audio_active) {
        audio_active = 1;
        audio_owner = current->pid;
        audio_load_next_locked();
    }
}

static void audio_timer_fn(int unused)
{
    flag_t flags;

    unused = unused;

    save_flags(flags);
    clr_irq();
    audio_timer_on = 0;

    if (audio_active)
        audio_load_next_locked();

    restore_flags(flags);
}

int audio_seq_ioctl(char *arg)
{
    struct audio_seq req;
    struct audio_event ev;
    struct audio_event *up;
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

    if (req.flags & (AUDIO_SEQ_F_FLUSH | AUDIO_SEQ_F_STOP))
        audio_stop_locked();

    if (req.flags & AUDIO_SEQ_F_STOP) {
        restore_flags(flags);
        return 0;
    }

    if (audio_active && audio_owner != current->pid) {
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
        if (audio_queue_event(&ev)) {
            if (queued)
                audio_start_locked();
            restore_flags(flags);
            return queued ? (int)queued : -EAGAIN;
        }
        queued++;
        restore_flags(flags);
    }

    save_flags(flags);
    clr_irq();
    if (queued)
        audio_start_locked();
    restore_flags(flags);

    return (int)queued;
}
