/*
 * Sound Blaster /dev/dsp driver
 *
 * Playback only, mono, raw unsigned 8-bit PCM, through 8-bit ISA DMA on the
 * primary 8237 (channel 1 or 3), driven by a time constant and either of the
 * two DSP playback commands described below, so anything from a Sound Blaster
 * 1.0 upwards works.  There is no 16-bit DMA, no SB16 high-DMA mode, no
 * recording, no runtime mixer control and no MIDI.  Sample format conversion
 * belongs in user space.
 *
 * On an XT with a hard disk the card cannot keep its factory IRQ 5 and DMA 3:
 * the hard disk controller owns both, so jumper the card for IRQ 7 and DMA 1.
 * See Documentation/text/soundblaster.txt.
 *
 * A DSP 2.00 or later card plays through auto-init DMA (DSP 0x1C with 8237
 * mode 0x58 plus the channel) over the two-block buffer as a circular one, so
 * nothing is
 * reprogrammed between blocks and playback is gapless.  A Sound Blaster 1.x,
 * which has only the single-block command, and a card reached through an OPTi
 * MAD16, whose engine does not raise the per-block interrupt auto-init needs,
 * fall back to one block in flight at a time: write() then blocks until the
 * previous block has drained, and the gap between blocks is the interrupt
 * latency plus reprogramming the 8237 and the DSP.
 *
 * Completion is detected from the card's interrupt.  The 8237 status register
 * is deliberately never read: reading it clears the terminal-count latch for
 * every channel on the chip, which would eat the completion the MFM disk
 * driver is waiting for on channel 3.  If a card whose interrupt is never
 * delivered has to be supported, poll the DMA count register instead, which
 * has no such side effect.  Until then a wrong irq= shows up as a per-block
 * timeout rather than working silently.
 *
 * /bootopts sb=irq,port,dma,flags overrides the compiled-in settings, and
 * sb=off skips the driver entirely.  The flag bits are the ISAF_ defines in
 * arch/audio-sb.h; ISAF_MAD16 asks for the OPTi 82C929 bring-up in audio-mad.c
 * before the card is probed, because such a card does not answer DSP commands
 * until its MC registers present the Sound Blaster personality.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same
 * terms as the ELKS kernel.
 */

#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/memory.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/audio.h>
#include <linuxmt/init.h>
#include <arch/io.h>
#include <arch/audio-sb.h>
#include <arch/irq.h>
#include <arch/dma.h>
#include <arch/segment.h>
#include <arch/divmod.h>

/*
 * All port access goes through the inb_p/outb_p bus-recovery forms.  Slow ISA
 * parts drop back-to-back accesses, and the 8237 address and count are written
 * as flip-flop pairs, so one lost byte points DMA at the wrong memory.
 */

/*
 * Default playback levels, in percent, quantised by sb_mixer_lr_byte() to the
 * card's 3- or 4-bit level fields: 71% is 5 of the 7 steps an SB Pro offers
 * and 11 of 15 on an SB16.  Master and voice attenuate in series, so leaving
 * both a couple of steps below full scale keeps hot 8-bit material out of the
 * clipping the top of the range brings on some cards.  FM is muted because
 * nothing here programs the OPL, so it only contributes its idle noise floor.
 * Some cards do not implement the don't-care bits of the packing, so never
 * read-modify-write mixer registers.  audiomix(1) changes levels at runtime.
 */
#define SB_DEFAULT_MASTERVOL 71     /* 5/7 */
#define SB_DEFAULT_PLAYVOL   71     /* 5/7 */
#define SB_DEFAULT_FMVOL     0

/*
 * SB_BOUNCE is one DMA block and is what SNDCTL_DSP_GETBLKSIZE reports.  Two
 * of them are claimed as a single buffer so the next block can be copied in
 * while the current one is still playing, leaving only the 8237 and DSP
 * reprogramming between blocks.
 */
#define SB_BOUNCE       SB_BUFSIZE
#define SB_BUFFER       (2 * SB_BOUNCE)
#define SB_MIN_RATE     4000U
/* documented ceiling for DSP 0x14 without the high-speed commands */
#define SB_MAX_RATE     20000U
#define SB_TC_CLOCK     1000000UL   /* time constant reference clock */

/* LPT1 shares IRQ 7 with the card; bit 4 of its control register is IRQ enable */
#define LPT1_CONTROL    0x37A
#define LPT1_CTRL_IDLE  0x0C        /* INIT + SELECT_IN, interrupt disabled */

static unsigned int sb_base;
static unsigned char sb_dma;
static unsigned char sb_irq_line;
static unsigned char sb_dsp_ver_major;
static unsigned char sb_dsp_ver_minor;
static unsigned char sb_present;
static unsigned char sb_opened;

static unsigned int sb_rate = 8000;
static unsigned char sb_timeconst;
static unsigned int sb_full_play_ticks;  /* <= 103 by construction: fits 16 bits */

/* cached 8237 ports for the configured channel */
static unsigned int sb_dma_addr_port;
static unsigned int sb_dma_page_reg;
static unsigned int sb_dma_count_port;
static unsigned char sb_dma_mask;

static segment_s *sb_bounce_seg;
static unsigned int sb_bounce_dma_off;   /* A15..A0 of the buffer */
static unsigned char sb_bounce_dma_page; /* A23..A16 of the buffer */
static unsigned char sb_fill;            /* half to copy into next, 0 or 1 */

static unsigned char sb_active;          /* a single-block transfer is playing */
static unsigned int sb_active_len;
static jiff_t sb_active_min_done;        /* earliest believable completion */
static volatile unsigned char sb_dma_done;
static struct wait_queue sb_wait;

/*
 * Auto-init (gapless) playback state, used on a DSP 2.00+ card instead of the
 * single-block path.  The 8237 runs continuously over the whole two-half
 * buffer and the DSP interrupts after each half, so nothing is reprogrammed
 * between halves and there is no inter-block gap.  sb_queued counts halves the
 * writer has filled but the card has not finished; the interrupt frees one and
 * flags sb_underrun if the writer had none ready.
 */
static unsigned char sb_mad16_route;     /* card is an OPTi MAD16 in SB mode */
static unsigned char sb_ai_active;       /* continuous auto-init DMA running */

static volatile unsigned char sb_queued; /* filled halves not yet drained (0..2) */
static volatile unsigned char sb_underrun;

/* GETERROR payload: 104 bytes, far too big for the 640-byte kernel stack */
static struct audio_errinfo sb_errinfo;
static __s32 sb_play_underruns;

/*
 * ceil(len * HZ / byte_rate) using the kernel's 32-bit divide helper rather
 * than a hand-rolled 16-bit long division.  len * HZ overflows 16 bits for any
 * buffer above 655 bytes, so the numerator has to be long.
 */
static unsigned int FARPROC sb_ceil_play_ticks(unsigned int len,
                                               unsigned int byte_rate)
{
    unsigned int rem = byte_rate;
    unsigned int ticks;

    if (!byte_rate)
        return 1;
    ticks = (unsigned int)__divmod((unsigned long)len * HZ, &rem);
    if (rem)
        ticks++;
    return ticks? ticks: 1;
}

/* Time constant the DSP wants for a byte rate: 256 - 1000000/rate. */
static unsigned char FARPROC sb_timeconst_for(unsigned int byte_rate)
{
    unsigned int rem = byte_rate;
    unsigned int div;

    if (!byte_rate)
        return 0;
    div = (unsigned int)__divmod(SB_TC_CLOCK, &rem);
    if (div > 255U)
        div = 255U;
    return (unsigned char)(256U - div);
}

static void FARPROC sb_rate_cache(unsigned int rate)
{
    sb_rate = rate;
    sb_timeconst = sb_timeconst_for(rate);
    sb_full_play_ticks = sb_ceil_play_ticks(SB_BOUNCE, rate);
}

/*
 * Rate the card will really run at, which is what the caller gets back from
 * SNDCTL_DSP_SPEED: the time constant quantises the divisor, so a requested
 * 11025 Hz comes back as something a percent or so away.
 */
static unsigned int FARPROC sb_actual_rate(void)
{
    unsigned int rem = (unsigned int)(256U - sb_timeconst);

    return (unsigned int)__divmod(SB_TC_CLOCK, &rem);
}

/*
 * 16 bits on purpose: the count is bounded by ceil(SB_BOUNCE*HZ/4000) = 103,
 * so carrying it as jiff_t forced 32-bit register-pair arithmetic through
 * every block start.  It is widened exactly where it meets jiffies().
 */
static unsigned int FARPROC sb_play_ticks(unsigned int len)
{
    if (len == SB_BOUNCE)
        return sb_full_play_ticks;
    return sb_ceil_play_ticks(len, sb_rate);
}

static int FARPROC sb_time_reached(jiff_t when)
{
    jiff_t now = jiffies();

    return now == when || time_after(now, when);
}

/*
 * A block cannot legitimately finish before its samples have had time to play.
 * With a single buffer, honouring an early or spurious interrupt would hand the
 * buffer back while the card was still reading it.
 */
static int FARPROC sb_completion_mature(void)
{
    return !sb_active || sb_time_reached(sb_active_min_done);
}

static int FARPROC dsp_wait_w(void)
{
    jiff_t deadline = jiffies() + (HZ / 10) + 1;

    do {
        if ((inb_p(sb_base + SB_WRITE_DATA) & 0x80) == 0)
            return 0;
        inb_p(0x61);            /* delay: a read plus its recovery cycle */
    } while (!time_after(jiffies(), deadline));

    return -EIO;
}

static int FARPROC dsp_cmd(unsigned char v)
{
    if (dsp_wait_w() < 0)
        return -EIO;
    outb_p(v, sb_base + SB_WRITE_DATA);
    return 0;
}

static int FARPROC dsp_read_byte(unsigned char *value)
{
    jiff_t deadline = jiffies() + (HZ / 10) + 1;

    do {
        if ((inb_p(sb_base + SB_READ_STATUS) & 0x80) != 0) {
            *value = inb_p(sb_base + SB_READ_DATA);
            return 0;
        }
        inb_p(0x61);
    } while (!time_after(jiffies(), deadline));

    return -EIO;
}

static int INITPROC sb_reset(void)
{
    int i;

    outb_p(1, sb_base + SB_RESET);
    for (i = 0; i < 200; i++)       /* the DSP needs 3us of reset */
        inb_p(0x61);
    outb_p(0, sb_base + SB_RESET);
    for (i = 0; i < 500; i++) {
        if ((inb_p(sb_base + SB_READ_STATUS) & 0x80) != 0) {
            if (inb_p(sb_base + SB_READ_DATA) == DSP_READY)
                return 0;
        }
    }
    return -ENODEV;
}

static int INITPROC sb_read_dsp_version(void)
{
    if (dsp_cmd(DSP_GET_VERSION) < 0)
        return -EIO;
    if (dsp_read_byte(&sb_dsp_ver_major) < 0 ||
        dsp_read_byte(&sb_dsp_ver_minor) < 0) {
        sb_dsp_ver_major = 0;
        sb_dsp_ver_minor = 0;
        return -EIO;
    }
    return 0;
}

static void INITPROC sb_dma_cache(void)
{
    if (sb_dma == 1) {
        sb_dma_addr_port = DMA_ADDR_1;
        sb_dma_count_port = DMA_CNT_1;
        sb_dma_page_reg = DMA_PAGE_1;
    } else {
        sb_dma_addr_port = DMA_ADDR_3;
        sb_dma_count_port = DMA_CNT_3;
        sb_dma_page_reg = DMA_PAGE_3;
    }
    sb_dma_mask = sb_dma;
}

/*
 * The 8237 wants a page byte and a 16-bit offset.  LINADDR resolves a segment
 * in real mode and a selector under CONFIG_286_PMODE, so it is the only correct
 * way to get a physical address out of a segment_s: seg->base is a selector in
 * protected mode and shifting it left by 4 would point at nothing.
 */
static void INITPROC sb_dma_addr_cache(addr_t phys)
{
    sb_bounce_dma_off = (unsigned int)phys;
    sb_bounce_dma_page = (unsigned char)(phys >> 16);
}

/*
 * A single 8237 transfer cannot step across a physical 64K page.  The whole
 * two-block buffer is checked, not one block, so that adding a half-buffer
 * offset to the cached low word can never carry into the page byte.
 */
static int INITPROC sb_dma_unusable(addr_t phys)
{
    if (phys + (addr_t)SB_BUFFER > 0x100000UL)      /* must be DMA-able */
        return 1;
    return ((phys & 0xffffUL) + (addr_t)SB_BUFFER - 1) > 0xffffUL;
}

/* Caller holds interrupts off: the address flip-flop is shared chip state. */
static void FARPROC sb_dma_program(unsigned int off, unsigned int len)
{
    union {
        unsigned int word;
        unsigned char byte[2];
    } address, count;

    address.word = sb_bounce_dma_off + off;
    count.word = len - 1U;              /* the 8237 is loaded with count-1 */

    outb_p(sb_dma_mask | 4, DMA1_MASK_REG);       /* mask the channel */
    outb_p(0, DMA1_CLEAR_FF_REG);
    outb_p(DMA_MODE_WRITE | sb_dma, DMA1_MODE_REG);
    outb_p(address.byte[0], sb_dma_addr_port);
    outb_p(address.byte[1], sb_dma_addr_port);
    outb_p(sb_bounce_dma_page, sb_dma_page_reg);
    outb_p(count.byte[0], sb_dma_count_port);
    outb_p(count.byte[1], sb_dma_count_port);
    outb_p(sb_dma_mask, DMA1_MASK_REG);           /* unmask, transfer armed */
}

static void FARPROC sb_dma_stop(void)
{
    outb_p(sb_dma_mask | 4, DMA1_MASK_REG);
}

static int FARPROC sb_dsp_start(unsigned int len)
{
    unsigned int n = len - 1U;

    /*
     * The time constant is resent before every block: a real SB DSP latches
     * it across 0x14 transfers, but some compatibles (OPTi 82C929) have only
     * been proven reliable with the per-block resend, and two dsp_cmd
     * handshakes per block is a small price.
     */
    if (dsp_cmd(DSP_SET_RATE) < 0 || dsp_cmd(sb_timeconst) < 0)
        return -EIO;
#ifdef CONFIG_AUDIO_MAD
    /*
     * The 929's SB engine rewrites the codec format register to mu-law
     * whenever it maps a rate, and the rate is resent for every block
     * (the OPTi has only proven reliable that way) - so the format has
     * to be repaired just as often.
     */
    if (sb_mad16_route)
        mad16_codec_fix_fmt();
#endif
    if (dsp_cmd(DSP_DMA_OUT_8) < 0 ||
        dsp_cmd((unsigned char)(n & 0xFF)) < 0 ||
        dsp_cmd((unsigned char)(n >> 8)) < 0)
        return -EIO;
    return 0;
}

static void FARPROC sb_idle(void)
{
    sb_dma_stop();
    sb_active = 0;
    sb_active_len = 0;
    sb_dma_done = 0;
    sb_active_min_done = 0;
}

static void sb_dsp_irq_ack(void);       /* drains the card's interrupt read path */

/*
 * Auto-init needs DSP 2.00, so a real SB 1.x keeps the single-block path.  So
 * does a MAD16: measured on an 82C929A, its SB engine plays continuous
 * auto-init badly where the same stream is clean single-block, which is the
 * same per-block hand-holding its time constant already needs.
 */
static int FARPROC sb_can_autoinit(void)
{
    if (sb_dsp_ver_major < 2)
        return 0;
    if (sb_mad16_route)
        return 0;
    return 1;
}

/*
 * Program the 8237 once over the whole two-half buffer with the auto-init bit
 * set, so at terminal count it reloads base and count itself and wraps back to
 * the start without the CPU touching it.  The DSP block-size command then makes
 * the card interrupt at each half boundary.
 */
static void FARPROC sb_dma_program_auto(void)
{
    union {
        unsigned int word;
        unsigned char byte[2];
    } count;

    count.word = SB_BUFFER - 1U;

    outb_p(sb_dma_mask | 4, DMA1_MASK_REG);       /* mask the channel */
    outb_p(0, DMA1_CLEAR_FF_REG);
    outb_p((DMA_MODE_WRITE | 0x10) | sb_dma, DMA1_MODE_REG);  /* +auto-init */
    outb_p((unsigned char)(sb_bounce_dma_off & 0xFF), sb_dma_addr_port);
    outb_p((unsigned char)(sb_bounce_dma_off >> 8), sb_dma_addr_port);
    outb_p(sb_bounce_dma_page, sb_dma_page_reg);
    outb_p(count.byte[0], sb_dma_count_port);
    outb_p(count.byte[1], sb_dma_count_port);
    outb_p(sb_dma_mask, DMA1_MASK_REG);           /* unmask, transfer armed */
}

/*
 * Start continuous playback: arm the 8237, tell the DSP the half size, then
 * kick off the 8-bit auto-init output.  From here the card runs on its own and
 * only interrupts at each half; sb_ai_queue refills behind the play point.
 */
static int FARPROC sb_ai_start(void)
{
    unsigned int blk = SB_BOUNCE - 1U;

    sb_dma_program_auto();
    if (dsp_cmd(DSP_SET_RATE) < 0 || dsp_cmd(sb_timeconst) < 0)
        return -EIO;
    if (dsp_cmd(DSP_DMA_BLKSIZE) < 0 ||
        dsp_cmd((unsigned char)(blk & 0xFF)) < 0 ||
        dsp_cmd((unsigned char)(blk >> 8)) < 0)
        return -EIO;
    if (dsp_cmd(DSP_DMA_OUT_8AI) < 0)
        return -EIO;
    (void)dsp_cmd(DSP_SPEAKER_ON);
    sb_ai_active = 1;
    return 0;
}

/* Stop continuous playback and return to the idle, nothing-queued state. */
static void FARPROC sb_ai_halt(void)
{
    unsigned int flags;
    unsigned char was_active = sb_ai_active;

    /* Mask the channel first so DRQ stops before the DSP is touched. */
    save_flags(flags);
    clr_irq();
    sb_dma_stop();
    sb_ai_active = 0;
    sb_queued = 0;
    sb_underrun = 0;
    sb_fill = 0;
    restore_flags(flags);

    if (was_active) {
        (void)dsp_cmd(DSP_HALT_DMA);        /* stop the transfer in flight */
        (void)dsp_cmd(DSP_DMA_EXIT_AI);     /* and leave auto-init mode */
        /*
         * Disconnect the DAC: halted, it holds its last sample as a DC
         * offset on the output.  Restarts reconnect it in sb_ai_start.
         */
        (void)dsp_cmd(DSP_SPEAKER_OFF);
    }
    sb_dsp_irq_ack();
}

/*
 * Copy one chunk into the free half and, on the first chunk, start the card.
 * Blocks while both halves are still queued, waking on the per-half interrupt.
 * A stalled source shows up as sb_underrun: the buffer was pre-filled with
 * silence, so the card played silence rather than a click, and playback is
 * resynced from a clean stop.
 */
static int FARPROC sb_ai_queue(char *buf, unsigned int len)
{
    unsigned int off, flags;

    if (len == 0)
        return 0;
    if (len > SB_BOUNCE)
        return -EINVAL;
    if (verify_area(VERIFY_READ, buf, len) != 0)
        return -EFAULT;

    for (;;) {
        prepare_to_wait_interruptible(&sb_wait);
        if (sb_underrun) {                  /* fell behind: restart clean */
            finish_wait(&sb_wait);
            sb_ai_halt();
            sb_play_underruns++;
            break;
        }
        if (sb_queued < 2) {
            finish_wait(&sb_wait);
            break;
        }
        if (current->signal) {
            finish_wait(&sb_wait);
            return -EINTR;
        }
        current->timeout = jiffies() + sb_full_play_ticks * 2 + (2 * HZ) + 1;
        do_wait();
        current->timeout = 0;
        finish_wait(&sb_wait);
    }

    off = sb_fill? SB_BOUNCE: 0;
    fmemcpyb((void *)off, sb_bounce_seg->base, buf, current->t_regs.ds, len);
    if (len < SB_BOUNCE)                     /* pad the tail with silence */
        fmemsetb((void *)(off + len), sb_bounce_seg->base, 0x80,
                 SB_BOUNCE - len);

    save_flags(flags);
    clr_irq();
    sb_queued++;
    sb_fill ^= 1;
    restore_flags(flags);

    if (!sb_ai_active) {
        int ret = sb_ai_start();
        if (ret < 0) {
            sb_ai_halt();
            sb_play_underruns++;
            return ret;
        }
    }
    return (int)len;
}

/* Wait for both queued halves to finish, so close/SYNC do not clip the tail. */
static void FARPROC sb_ai_drain(void)
{
    jiff_t deadline = jiffies() + sb_full_play_ticks * 4 + (2 * HZ) + 1;

    while (sb_ai_active && sb_queued > 0 && !sb_underrun) {
        prepare_to_wait_interruptible(&sb_wait);
        if (!sb_ai_active || sb_queued == 0 || sb_underrun || current->signal) {
            finish_wait(&sb_wait);
            break;
        }
        if (time_after(jiffies(), deadline)) {
            finish_wait(&sb_wait);
            break;
        }
        current->timeout = deadline + 1;
        do_wait();
        current->timeout = 0;
        finish_wait(&sb_wait);
    }
    fmemsetb((void *)0, sb_bounce_seg->base, 0x80, SB_BUFFER);
    sb_ai_halt();
}

static int FARPROC sb_start(unsigned int off, unsigned int len)
{
    unsigned int flags;
    unsigned int ticks;
    int ret;

    ticks = sb_play_ticks(len);

    save_flags(flags);
    clr_irq();
    sb_dma_done = 0;
    sb_dma_program(off, len);
    sb_active = 1;
    sb_active_len = len;
    sb_active_min_done = jiffies() + ((ticks > 1)? ticks - 1: 0);
    restore_flags(flags);

    ret = sb_dsp_start(len);
    if (ret < 0) {
        save_flags(flags);
        clr_irq();
        sb_idle();
        sb_play_underruns++;
        restore_flags(flags);
    }
    return ret;
}

static void FARPROC sb_halt(void)
{
    unsigned int flags;

    if (sb_ai_active) {
        sb_ai_halt();
        return;
    }
    (void)dsp_cmd(DSP_HALT_DMA);
    save_flags(flags);
    clr_irq();
    sb_idle();
    sb_fill = 0;
    restore_flags(flags);
}

/*
 * The mixer index and data ports need a real settling delay between them:
 * back-to-back writes are lost entirely on some compatibles (OPTi 82C929).
 * Mixer writes only happen at init and open, so erring long costs nothing.
 */
#define SB_MIXER_DELAY  1200        /* ~3ms: each inb_p is a read plus a recovery write */

static void FARPROC sb_mixer_delay(void)
{
    int i;

    for (i = 0; i < SB_MIXER_DELAY; i++)
        inb_p(0x61);
}

static void FARPROC sb_mixer_write(unsigned char reg, unsigned char value)
{
    outb_p(reg, sb_base + SB_MIXER_ADDR);
    sb_mixer_delay();
    outb_p(value, sb_base + SB_MIXER_DATA);
    sb_mixer_delay();
}

/* A Sound Blaster 1.x or 2.0 has no mixer at all. */
static int FARPROC sb_has_mixer(void)
{
    return sb_dsp_ver_major == 0 || sb_dsp_ver_major >= 3;
}

/*
 * SB Pro output mode: mono only, with the ~3.2kHz output filter engaged at
 * low rates and bypassed above 8kHz, where it would discard most of what the
 * higher rate was for.  Only DSP 3.xx keeps this in the mixer: a plain SB has
 * no mixer and the SB16 takes its channel count from the DSP command.  The
 * register is written outright rather than read-modify-written because some
 * compatibles (MAD16) do not read mixer registers back faithfully.
 */
static void FARPROC sb_set_output_mode(void)
{
    if (sb_has_mixer())
        sb_mixer_write(SB_MIX_OUTFILT,
            (unsigned char)((sb_rate > 8000U)? SB_FILT_OFF: 0x00));
}

/*
 * Cards can come out of reset with the voice or master level at zero, which
 * looks exactly like a driver that runs but produces no sound, so both are
 * always programmed.  The FM input is muted: nothing initialises the FM
 * section, and it is summed into the same output at whatever level the card
 * powered up with.
 */
static unsigned char FARPROC sb_mixer_lr_byte(unsigned int l, unsigned int r)
{
    if (sb_dsp_ver_major >= 4)          /* SB16: 4-bit fields, 7-4 and 3-0 */
        return (unsigned char)((((l * 15U + 50U) / 100U) << 4) |
                                ((r * 15U + 50U) / 100U));
    /* SB Pro and MAD16: 3-bit fields at 7-5 and 3-1 */
    return (unsigned char)((((l * 7U + 50U) / 100U) << 5) |
                           (((r * 7U + 50U) / 100U) << 1));
}

static void FARPROC sb_mixer_program(void)
{
    if (!sb_has_mixer())
        return;
    sb_mixer_write(SB_MIX_MASTER, sb_mixer_lr_byte(SB_DEFAULT_MASTERVOL,
                                                   SB_DEFAULT_MASTERVOL));
    sb_mixer_write(SB_MIX_VOICE, sb_mixer_lr_byte(SB_DEFAULT_PLAYVOL,
                                                  SB_DEFAULT_PLAYVOL));
    sb_mixer_write(SB_MIX_FM, sb_mixer_lr_byte(SB_DEFAULT_FMVOL,
                                               SB_DEFAULT_FMVOL));
    /*
     * Every input the driver does not use is muted outright.  A DSP reset
     * restores the card's own defaults for all of these, the readback path
     * cannot confirm anything on this chip (reads OR in 0x11), and any
     * input left at a default level is summed into the output as noise.
     */
    sb_mixer_write(SB_MIX_MIC, 0x00);
    sb_mixer_write(SB_MIX_CD, 0x00);
    sb_mixer_write(SB_MIX_LINE, 0x00);
    sb_set_output_mode();
}

/*
 * Programmed again on every open rather than only at init.  A DSP reset makes
 * this card restore its own mixer defaults, which puts the FM input back to 60%
 * and undoes the mute, so anything that has touched the card since boot leaves
 * state we have to reassert rather than assume.
 */
/*
 * SB Pro, MAD16 and many compatibles hold their interrupt asserted until the
 * DSP read path is serviced, so drain it rather than just reading status once.
 */
static void sb_dsp_irq_ack(void)
{
    unsigned int n = 0;

    while (n < 8 && (inb_p(sb_base + SB_READ_STATUS) & 0x80) != 0) {
        inb_p(sb_base + SB_READ_DATA);
        n++;
    }
}

/*
 * Must not be FARPROC: request_irq stores a near pointer and the trampoline
 * calls it with KERNEL_CS.  The PIC end-of-interrupt is issued by the irqit
 * wrapper, so this must not touch the PIC.  It can also be entered with no
 * transfer running, both from a PIC spurious interrupt on irq 7 and from a
 * card that interrupts after sb_halt().
 */
static void sb_interrupt(int irq, struct pt_regs *regs)
{
    sb_dsp_irq_ack();
    if (sb_ai_active) {                 /* a half finished under auto-init */
        if (sb_queued > 0)
            sb_queued--;
        else
            sb_underrun = 1;            /* writer had nothing ready */
        wake_up(&sb_wait);
        return;
    }
    if (!sb_active) {
        return;
    }
    sb_dma_done = 1;
    wake_up(&sb_wait);
}

static int FARPROC sb_block_complete(void)
{
    if (!sb_dma_done)
        return 0;
    return sb_completion_mature();
}

/*
 * Wait for the block in flight.  The deadline is only a backstop against a
 * card whose interrupt never arrives and is deliberately generous: emulated
 * cards are driven by the host audio stack and can stall DMA for a large
 * fraction of a second.  Erring long only changes how quickly a mis-set irq=
 * reports failure; playback itself always ends on the interrupt.
 */
static int FARPROC sb_wait_complete(unsigned int len)
{
    jiff_t entry = jiffies();
    jiff_t deadline = entry + sb_play_ticks(len) * 2 + (2 * HZ) + 1;
    unsigned int flags;

    for (;;) {
        prepare_to_wait_interruptible(&sb_wait);
        if (sb_block_complete()) {
            finish_wait(&sb_wait);
            break;
        }
        if (current->signal) {
            finish_wait(&sb_wait);
            sb_halt();
            return -EINTR;
        }
        if (time_after(jiffies(), deadline)) {
            finish_wait(&sb_wait);
            sb_play_underruns++;
            sb_halt();
            return -EIO;
        }
        /*
         * Sleep to the next actual event, not the next clock tick.  The
         * completion interrupt wakes this task through sb_wait; the timer is
         * only needed for an early interrupt gated by sb_completion_mature()
         * (no further interrupt will come, so wake at the maturity time) and
         * for the no-interrupt backstop.  Sleeping in 1-jiffy slices here
         * cost about fifty scheduler round trips per block.  A deadline that
         * has just slipped past is safe: schedule() treats timeout <= jiffies
         * as already expired and keeps the task runnable.
         */
        current->timeout = (sb_dma_done? sb_active_min_done: deadline) + 1;
        do_wait();
        current->timeout = 0;
        finish_wait(&sb_wait);
    }

    save_flags(flags);
    clr_irq();
    sb_idle();
    restore_flags(flags);
    sb_dsp_irq_ack();
    return 0;
}

/*
 * Copy into the idle half first, then wait for the playing half.  sb_fill only
 * advances once the half it names has been handed to the card, so it always
 * names the half the 8237 is not reading, and the copy overlaps playback of the
 * other one.  When the wait returns, the next block is already in memory and
 * only needs the chip programmed.
 */
static int FARPROC sb_queue_chunk(char *buf, unsigned int len)
{
    unsigned int off;
    int ret;

    if (sb_can_autoinit())         /* gapless path on a DSP 2.00+ card */
        return sb_ai_queue(buf, len);

    if (len > SB_BOUNCE)            /* would overrun one half of the buffer */
        return -EINVAL;
    if (verify_area(VERIFY_READ, buf, len) != 0)
        return -EFAULT;

    off = sb_fill? SB_BOUNCE: 0;
    fmemcpyb((void *)off, sb_bounce_seg->base, buf, current->t_regs.ds, len);

    if (sb_active) {
        ret = sb_wait_complete(sb_active_len);
        if (ret < 0)
            return ret;
    }
    ret = sb_start(off, len);
    if (ret < 0)
        return ret;
    sb_fill ^= 1;
    return 0;
}

static int FARPROC sb_open_impl(struct inode *inode, struct file *file)
{
    if (!sb_present || !sb_bounce_seg)
        return -ENODEV;
    if (MINOR(inode->i_rdev) != 0)
        return -ENODEV;
    if (sb_opened)
        return -EBUSY;

#ifdef CONFIG_AUDIO_MAD
    /* something else may have moved the SB personality since we set it up */
    mad16_restore_profile();
#endif
    (void)dsp_cmd(DSP_SPEAKER_ON);
    sb_mixer_program();         /* a DSP reset may have undone all of it */
    sb_play_underruns = 0;
    sb_fill = 0;
    sb_rate_cache(sb_rate);
    sb_idle();
    sb_ai_active = 0;
    sb_queued = 0;
    sb_underrun = 0;
    /*
     * Pre-fill the whole buffer with unsigned-8 silence so that if the writer
     * ever falls behind under auto-init the card replays silence, not a click
     * or whatever last passed through.
     */
    fmemsetb((void *)0, sb_bounce_seg->base, 0x80, SB_BUFFER);
    sb_opened = 1;
    return 0;
}

static void FARPROC sb_release_impl(struct inode *inode, struct file *file)
{
    if (sb_ai_active)
        sb_ai_drain();          /* let queued halves finish, then stop */
    else {
        if (sb_active)
            (void)sb_wait_complete(sb_active_len);
        sb_halt();
    }
    sb_opened = 0;
}

/* No capture path, so a reader gets end of file rather than an error. */
static size_t sb_read(struct inode *inode, struct file *file, char *buf,
                      size_t count)
{
    return 0;
}

/*
 * Loop internally over DMA-sized blocks so a caller that ignores the return
 * value still plays all of its data.  A partial write reports the bytes that
 * were accepted; the error is only returned when nothing at all was written,
 * which is what lets a caller safely retry on EINTR.
 */
static size_t FARPROC sb_write_impl(struct inode *inode, struct file *file,
                                    char *buf, size_t count)
{
    size_t done = 0;
    unsigned int chunk;
    int ret;

    if (!(file->f_mode & FMODE_WRITE))
        return -EINVAL;

    while (done < count) {
        chunk = (unsigned int)(count - done);
        if (chunk > SB_BOUNCE)
            chunk = SB_BOUNCE;
        ret = sb_queue_chunk(buf + done, chunk);
        if (ret < 0)
            return done? done: (size_t)ret;
        done += chunk;
    }
    return done;
}

static int FARPROC sb_get_arg(char *arg, void *dst, size_t len)
{
    if (!arg)
        return -EINVAL;
    return verified_memcpy_fromfs(dst, arg, len)? -EFAULT: 0;
}

static int FARPROC sb_put_arg(char *arg, void *src, size_t len)
{
    if (!arg)
        return -EINVAL;
    return verified_memcpy_tofs(arg, src, len)? -EFAULT: 0;
}

static int FARPROC sb_ioctl_impl(struct inode *inode, struct file *file,
                                 int cmd, char *arg)
{
    __s32 val;
    unsigned int rate;
    int ret;

    switch (cmd) {
    case SNDCTL_DSP_RESET:
        sb_halt();
        return 0;

    case SNDCTL_DSP_SYNC:
        if (sb_ai_active) {
            sb_ai_drain();
            return 0;
        }
        if (sb_active) {
            ret = sb_wait_complete(sb_active_len);
            if (ret < 0)
                return ret;
        }
        sb_halt();
        return 0;

    case SNDCTL_DSP_POST:
        return 0;

    case SNDCTL_DSP_SPEED:
        ret = sb_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val > 0) {                  /* 0 means read the current rate */
            unsigned int ceiling = SB_MAX_RATE;

            rate = (val > (__s32)ceiling)? ceiling:
                   (val < (__s32)SB_MIN_RATE)? SB_MIN_RATE:
                   (unsigned int)val;
            if (rate != sb_rate && sb_ai_active)
                sb_ai_halt();           /* restart at the new rate on next write */
            sb_rate_cache(rate);
        }
        val = (__s32)sb_actual_rate();
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_SETFMT:
        ret = sb_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != (__s32)DSP_FMT_QUERY && val != (__s32)DSP_FMT_U8)
            return -EINVAL;
        val = (__s32)DSP_FMT_U8;
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETFMTS:
        val = (__s32)DSP_FMT_U8;
        return sb_put_arg(arg, &val, sizeof(val));

    /*
     * Mono only: accept a request for one channel, reject two.  Both of these
     * program the hardware rather than just agreeing with the caller, because
     * on the SB Pro the channel count lives in a mixer register and reporting
     * mono while the card is still wired for stereo is how a mono stream ends
     * up played at double speed.
     */
    case SNDCTL_DSP_CHANNELS:
        ret = sb_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != 0 && val != 1)
            return -EINVAL;
        sb_set_output_mode();
        val = 1;
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_STEREO:
        ret = sb_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != 0)
            return -EINVAL;
        sb_set_output_mode();
        val = 0;
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETBLKSIZE:
        val = (__s32)SB_BOUNCE;
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETERROR:
        memset(&sb_errinfo, 0, sizeof(sb_errinfo));
        sb_errinfo.play_underruns = sb_play_underruns;
        sb_play_underruns = 0;
        return sb_put_arg(arg, &sb_errinfo, sizeof(sb_errinfo));
    }

    return -EINVAL;
}

/*
 * The file_operations slots have to be near-callable, so these thin wrappers
 * are what keeps the body of the driver in far text.
 */
static int sb_open(struct inode *inode, struct file *file)
{
    return sb_open_impl(inode, file);
}

static void sb_release(struct inode *inode, struct file *file)
{
    sb_release_impl(inode, file);
}

static size_t sb_write(struct inode *inode, struct file *file, char *buf,
                       size_t count)
{
    return sb_write_impl(inode, file, buf, count);
}

static int sb_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    return sb_ioctl_impl(inode, file, cmd, arg);
}

static struct file_operations sb_dsp_fops = {
    NULL,                       /* lseek */
    sb_read,                    /* read */
    sb_write,                   /* write */
    NULL,                       /* readdir */
    NULL,                       /* select */
    sb_ioctl,                   /* ioctl */
    sb_open,                    /* open */
    sb_release                  /* release */
};

/*
 * Claim a bounce buffer the 8237 can actually reach.  seg_alloc gives no
 * placement guarantee, so keep the rejects held until a usable block turns up,
 * which stops the allocator handing back the same unusable one.
 */
#define SB_ALLOC_TRIES  4

static addr_t INITPROC sb_alloc_bounce(void)
{
    segment_s *reject[SB_ALLOC_TRIES];
    segment_s *seg;
    addr_t phys = 0;
    int nreject = 0;
    int i;

    for (i = 0; i < SB_ALLOC_TRIES; i++) {
        seg = seg_alloc((segext_t)((SB_BUFFER + 15) >> 4),
                        SEG_FLAG_EXTBUF | SEG_FLAG_ALIGN1K);
        if (!seg)
            break;
        phys = LINADDR(seg->base, 0);
        if (!sb_dma_unusable(phys)) {
            sb_bounce_seg = seg;
            break;
        }
        reject[nreject++] = seg;
    }
    while (nreject-- > 0)
        seg_put(reject[nreject]);

    return sb_bounce_seg? phys: 0;
}

void INITPROC dsp_init(void)
{
    struct isa_conf *conf = &sb_conf;
    addr_t phys;

    if (conf->port == -1)               /* sb=off */
        return;
    sb_base = (unsigned int)conf->port;
    sb_irq_line = (unsigned char)conf->irq;
    sb_dma = (unsigned char)conf->ram;
    /*
     * sb=irq,port,dma,1 selects the 8237 Extended Write strobe for a machine
     * whose DMA timing is tighter than 8-bit cards expect (the Amstrad
     * PC1512/1640).  Written once, here: the machine that needs it keeps it
     * across BIOS disk traffic, and rewriting the Amstrad ASIC's command
     * register between transfers audibly disturbs playback.
     */
    if (conf->flags & ISAF_EXTWRITE) {
        outb_p(DMA1_CMD_EXTWRITE, DMA1_CMD_REG);
        printk("sb: 8237 extended write enabled\n");
    }
    if (sb_dma != 1 && sb_dma != 3) {
        printk("sb: dma %d not 1 or 3\n", sb_dma);
        return;
    }
#ifdef CONFIG_BLK_DEV_MFMHD
    /*
     * An XT hard disk controller owns DMA channel 3 and IRQ 5.  Sharing the
     * channel would corrupt disk transfers, so refuse it outright; sharing the
     * interrupt only costs us spurious handler entries, so warn and continue.
     */
    if (sb_dma == 3) {
        printk("sb: dma 3 belongs to the hard disk controller, use dma 1\n");
        return;
    }
    if (sb_irq_line == 5)
        printk("sb: irq 5 belongs to the hard disk controller, prefer irq 7\n");
#endif
    sb_dma_cache();
    sb_rate_cache(sb_rate);

    phys = sb_alloc_bounce();
    if (!phys) {
        printk("sb: no dma buffer below 1M\n");
        return;
    }
    sb_dma_addr_cache(phys);

#ifdef CONFIG_AUDIO_MAD
    /*
     * An OPTi 82C929 is not a second sound card, it is this one wearing a
     * different hat: the chip does not answer DSP commands at all until its
     * MC registers have been told to present the Sound Blaster personality.
     * So the route is the sb= route - there is nothing separate to configure -
     * and the flag bit only says that this card needs the step performed.
     */
    if (conf->flags & ISAF_MAD16) {
        int rc = mad16_early_init(sb_base, (int)sb_irq_line, (int)sb_dma);

        /*
         * Distinguish the two failures: a card with no jumpers depends on this
         * step, so "which of the two went wrong" is the whole diagnostic.
         */
        if (rc == 0) {
            sb_mad16_route = 1;
            printk("sb: mad16 at 0x%x irq %d dma %d\n", sb_base, sb_irq_line,
                   sb_dma);
        }
        else if (rc == -EINVAL)
            printk("sb: mad16 cannot route 0x%x irq %d dma %d, "
                   "allows port 0x220/0x240 irq 5/7 dma 1/3\n", sb_base,
                   sb_irq_line, sb_dma);
        else
            printk("sb: mad16 82c929 not detected\n");
    }
#endif

    if (sb_reset() < 0) {
        printk("sb: no card at 0x%x\n", sb_base);
        goto out_free;
    }
    (void)sb_read_dsp_version();
    /*
     * IRQ 7 is LPT1's line as well as the card's, and the printer port powers
     * up with its interrupt enabled; a floating or strobed ACK then fires the
     * sound ISR at random points in a transfer, audible as distortion.  The
     * lp driver polls and never requests IRQ 7, so idle LPT1's control
     * register with the interrupt bit clear.  ISAF_LPTIRQ leaves the printer
     * port alone on a machine that really does drive LPT1 from its interrupt.
     */
    if (sb_irq_line == 7 && !(conf->flags & ISAF_LPTIRQ))
        outb_p(LPT1_CTRL_IDLE, LPT1_CONTROL);

    if (request_irq((int)sb_irq_line, sb_interrupt, INT_GENERIC)) {
        printk("sb: irq %d busy\n", sb_irq_line);
        goto out_free;
    }
    if (dsp_cmd(DSP_SPEAKER_ON) < 0) {
        printk("sb: dsp not responding\n");
        goto out_irq;
    }
    sb_mixer_program();

    if (register_chrdev(DSP_MAJOR, "dsp", &sb_dsp_fops)) {
        printk("sb: unable to register\n");
        goto out_irq;
    }
    sb_present = 1;
    printk("sb: dsp %d.%02d at 0x%x irq %d dma %d, %u byte buffer\n",
        sb_dsp_ver_major, sb_dsp_ver_minor, sb_base, sb_irq_line, sb_dma,
        (unsigned int)SB_BOUNCE);
    return;

out_irq:
    free_irq((int)sb_irq_line);
out_free:
    seg_put(sb_bounce_seg);
    sb_bounce_seg = NULL;
}
