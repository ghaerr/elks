/*
 * Sound Blaster /dev/dsp driver
 *
 * Playback only, mono, raw unsigned 8-bit PCM, through 8-bit ISA DMA on the
 * primary 8237 (channel 1 or 3) using the classic DSP 0x14 single-block command
 * and a time constant, so anything from a Sound Blaster 1.0 upwards works.
 * There is no 16-bit DMA, no SB16 high-DMA mode, no recording, no runtime mixer
 * control and no MIDI.  Sample format conversion belongs in user space.
 *
 * On an XT with a hard disk the card cannot keep its factory IRQ 5 and DMA 3:
 * the hard disk controller owns both of those, so jumper the card for IRQ 7 and
 * DMA 1.  The Amstrad PC1640 is typical here, assigning IRQ 5 to the hard disk
 * controller, DMA 3 to the external hard disk controller, and leaving DMA 1
 * spare for the expansion bus.
 *
 * One DMA block is in flight at a time and write() blocks until the previous
 * block has drained.  The bounce buffer holds two blocks so the next one is
 * copied in while the current one plays, which keeps the copy out of the window
 * when the DMA engine is stopped; what is left there is the interrupt latency
 * plus reprogramming the 8237 and the DSP.  Closing that remainder as well needs
 * auto-init DMA (DSP 0x1C with 8237 mode 0x58) over a circular buffer, which
 * also requires DSP 2.00 and so would drop Sound Blaster 1.0.
 *
 * Completion is detected from the card's interrupt.  The 8237 status register
 * is deliberately never read: reading it clears the terminal-count latch for
 * every channel on the chip, which would eat the completion the MFM disk
 * driver is waiting for on channel 3.  If a card whose interrupt is never
 * delivered has to be supported, poll the DMA count register instead, which
 * has no such side effect.  Until then a wrong irq= shows up as a per-block
 * timeout rather than working silently.
 *
 * /bootopts sb=port,irq,dma overrides the compiled-in settings, and sb=off
 * skips the driver entirely.
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_DSP

#include <linuxmt/types.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/memory.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/soundcard.h>
#include <linuxmt/init.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/dma.h>
#include <arch/segment.h>
#include <arch/divmod.h>

/*
 * Every port access in this driver goes through the bus-recovery forms.  The
 * xt-elks driver that worked on this machine built with CONFIG_XT_SLOW_ISA_IO=y
 * and routed all 54 of its accesses through inb_p/outb_p (sb_dsp.c:46-52); that
 * symbol does not exist in this tree, and these two sound files are the only
 * drivers left using bare inb/outb - lp, kbd-scancode, serial-8250, mfmhd and
 * directfd all still use the _p forms.
 *
 * On an 8 MHz Amstrad this matters.  Back-to-back accesses with no recovery
 * cycle are dropped or mis-latched by slow ISA parts: the 8237 programming
 * sequence writes the address and count as flip-flop pairs, so one lost byte
 * points DMA at the wrong memory, and the OPTi password gate and the AD1848
 * index/data pairs have the same exposure.  An emulator does not model bus
 * recovery, which is why this reads clean in 86Box and breaks on real hardware.
 */

/* DSP register offsets from the card base address */
#define SB_RESET        0x06        /* w  write 1 then 0 to reset the DSP */
#define SB_READ_DATA    0x0A        /* r  DSP data */
#define SB_WRITE_DATA   0x0C        /* rw bit 7 of read = write not ready */
#define SB_READ_STATUS  0x0E        /* r  bit 7 = data available, acks IRQ */
#define SB_MIXER_ADDR   0x04        /* w  mixer register select */
#define SB_MIXER_DATA   0x05        /* rw mixer register value */

/* DSP commands used here */
#define DSP_DIRECT_DAC  0x10        /* one sample straight to the DAC */
#define DSP_SET_RATE    0x40        /* followed by the time constant */
#define DSP_DMA_OUT_8   0x14        /* followed by length-1, single block */
#define DSP_HALT_DMA    0xD0
#define DSP_SPEAKER_ON  0xD1
#define DSP_GET_VERSION 0xE1
#define DSP_READY       0xAA        /* reset acknowledge byte */

/* Mixer registers, SB Pro layout, kept by the SB16 for compatibility */
#define SB_MIX_VOICE    0x04
#define SB_MIX_MIC      0x0A
#define SB_MIX_OUTFILT  0x0E        /* SB Pro output mode, not a level */
#define SB_MIX_MASTER   0x22
#define SB_MIX_FM       0x26
#define SB_MIX_CD       0x28
#define SB_MIX_LINE     0x2E

/*
 * SB_MIX_OUTFILT is where the SB Pro selects mono or stereo output; it is not
 * in the DSP command.  A card left in stereo mode takes two bytes per frame, so
 * a mono stream is consumed at twice the intended rate: it plays at double
 * speed with alternate samples going to opposite channels.  Linux does the
 * equivalent in sbpro_audio_prepare_for_output(), which calls
 * sb_mixer_set_stereo() on every playback rather than once at init.
 *
 * The filter bit polarity is the other way round from what the name suggests.
 * Linux's sb_mixer.h says so outright: "FILT_ON 0 - Yes, 0 to turn it on, 1 for
 * off".  Leaving it on is what an 8 kHz reconstruction wants.
 */
#define SB_MONO_DAC     0x00
#define SB_STEREO_DAC   0x02
#define SB_FILT_OFF     0x20
#define SB_OUTFILT_MONO (SB_MONO_DAC)   /* mono, output filter engaged */

/*
 * Playback levels, as percentages, matching the xt-elks driver that worked on
 * this hardware (SB_DEFAULT_MASTERVOL/SB_DEFAULT_PLAYVOL, sb_dsp.c:63-64).
 * sb_mixer_lr_byte() packs them for whichever layout the card uses.
 *
 * The master and voice attenuators are in series ahead of the output amplifier,
 * so both at full scale asks for the loudest analog path the card can produce
 * and 8-bit material that already peaks near full scale clips in the mixer.
 * These two values are the ones that were in use when playback was known good;
 * do not treat them as a free parameter without listening.
 *
 * A DSP 3.xx card packs each level as two 3-bit fields at bits 7-5 and 3-1 -
 * only the SB16 uses 4-bit fields.  That also explains why this card looks like
 * it ORs 0x11 into every mixer read: bits 4 and 0 are the don't-care bits of the
 * 3-bit packing and are simply not implemented, so a read can never confirm
 * them.  Do not read-modify-write these registers.
 *
 * sbmix(1) sets them live, which is how to find the right value for a
 * particular board without rebuilding.
 */
/*
 * These are percentages, quantised by sb_mixer_lr_byte to the eight steps a
 * 3-bit field has: (pct * 7 + 50) / 100.  100 lands on 7/7 and 85 on 6/7 - any
 * value from 93 to 100, and from 79 to 92 respectively, gives the same byte, so
 * these are mid-range choices rather than edge ones.
 */
/*
 * Master deliberately stays off full scale.  7/7 drives this card's output
 * stage into audible clipping with hot PCM material - confirmed by dropping
 * the register live during playback, which cleaned the sound up immediately -
 * and 5/7 is the level every "plays perfectly" verdict on this hardware was
 * actually given at.  Any percentage from 65 to 78 packs to the same 5/7 byte.
 */
#define SB_DEFAULT_MASTERVOL 85     /* 6/7 - full scale clips this card */
#define SB_DEFAULT_PLAYVOL   85     /* 6/7 */
/*
 * The FM input is turned right down because nothing here drives the OPL3, so
 * all it contributes is its idle noise floor.
 *
 * This has to be done from the driver, not from sbmix(1).  sbmix resets the DSP
 * on every run, and this card restores its own mixer defaults afterwards, which
 * puts FM back to 60% - so a level written by sbmix is undone before the tool
 * has even finished.  Writing it here, in the open path where no reset follows,
 * is what makes it stick: 0x26 then reads 0x11, which is 0/7 plus the two
 * don't-care bits of the 3-bit packing.
 */
#define SB_DEFAULT_FMVOL     0

/*
 * SB_BOUNCE is one DMA block and is what SNDCTL_DSP_GETBLKSIZE reports.  Two
 * of them are claimed as a single buffer so the next block can be copied in
 * while the current one is still playing.
 *
 * DSP 0x14 is a single-shot command, so a transfer has to be restarted for
 * every block, and the copy used to happen after the previous block had
 * drained.  That put a 4 KB far memcpy inside the window when nothing was
 * playing, which is milliseconds on an 8 MHz machine and was plainly audible
 * as a gap once per block: twice a second at 8000 Hz, five times a second at
 * 20000 Hz.  Filling the idle half in advance leaves only the 8237 and DSP
 * reprogramming between blocks.
 */
#define SB_BOUNCE       CONFIG_SB_BOUNCE
#define SB_BUFFER       (2 * SB_BOUNCE)
#define SB_MIN_RATE     4000U
/*
 * 20 kHz is below half CD rate, divides the 1 MHz time-constant clock exactly
 * and is the documented ceiling for DSP 0x14 without the high-speed commands.
 */
#define SB_MAX_RATE     20000U
/*
 * PIO playback is not bound by that ceiling.  It never issues DSP 0x14: every
 * sample goes out through the direct DAC command with the 8253 setting the
 * pace, so the limit is how fast the CPU can hand bytes over, not what the DSP
 * will accept.  22050 leaves an 8 MHz 8086 about 45us per sample against
 * roughly 12us of port I/O, which is comfortable; the rate is still quantised
 * to whole PIT ticks, so the reported rate is what will actually be produced.
 */
#define SB_MAX_RATE_PIO 22050U

#define SB_TC_CLOCK     1000000UL   /* time constant reference clock */

/* LPT1 shares IRQ 7 with the card; bit 4 of its control register is IRQ enable */
#define LPT1_CONTROL    0x37A
#define LPT1_CTRL_IDLE  0x0C        /* INIT + SELECT_IN, interrupt disabled */

/* /bootopts sb=port,irq,dma, parsed in init/main.c; -1 means not given */
extern int sb_port_opt;
extern int sb_irq_opt;
extern int sb_dma_opt;
extern int dma_extwrite_opt;    /* /bootopts dmaxw=1: 8237 Extended Write */
#ifdef CONFIG_SB_MAD16
extern int mad16_opt;
extern int mad16_port_opt;
extern int mad16_irq_opt;
extern int mad16_dma_opt;
#endif

static unsigned int sb_base;
static unsigned char sb_dma;
static unsigned char sb_irq_line;
static unsigned char sb_dsp_ver_major;
static unsigned char sb_dsp_ver_minor;
static unsigned char sb_present;
static unsigned char sb_opened;

static unsigned int sb_rate = 8000;
static unsigned char sb_timeconst;
/*
 * PIO playback, selected with sb=port,irq,0 in /bootopts.
 *
 * The Amstrad PC1512/1640 chipset does not service DRQ the way a true XT does -
 * mfmhd.c documents the same limitation for the hard disk on DRQ3, and on this
 * machine the sound card's DMA channel is audibly broken: DMA playback is
 * distorted while the DSP's direct DAC command, which touches no 8237 at all,
 * is clean.  In PIO mode every sample is handed to the DSP by the CPU.
 *
 * The cost is real and unavoidable: the CPU can do nothing else while a block
 * plays, because an 8086 has no time to spare between samples at 8 kHz.  The
 * write() call therefore runs to completion with interrupts still enabled but
 * the processor fully occupied, which is exactly what the hardware demands.
 */
static unsigned char sb_pio_mode;
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

static unsigned char sb_active;          /* a block is playing */
static unsigned int sb_active_len;
static jiff_t sb_active_min_done;        /* earliest believable completion */
static volatile unsigned char sb_dma_done;
static struct wait_queue sb_wait;

/* GETERROR payload: 104 bytes, far too big for the 640-byte kernel stack */
static struct audio_errinfo sb_errinfo;
static oss_int32_t sb_play_underruns;

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

/*
 * Pace samples off the 8253 rather than a calibrated delay loop.  Timer 0 runs
 * at 1.193182 MHz regardless of CPU speed, so counting its ticks gives the same
 * pitch on any machine, where a spin loop would have to be retuned per host and
 * would drift with cache and refresh.  The counter counts DOWN and wraps at
 * 65536, so elapsed ticks are (previous - current) & 0xFFFF.
 */
#define PIT_CH0         0x40
#define PIT_CMD         0x43
#define PIT_LATCH0      0x00        /* counter latch, channel 0 */

static unsigned int FARPROC pit_read(void)
{
    unsigned int flags, lo, hi;

    save_flags(flags);
    clr_irq();
    outb_p(PIT_LATCH0, PIT_CMD);
    lo = inb_p(PIT_CH0);
    hi = inb_p(PIT_CH0);
    restore_flags(flags);
    return (hi << 8) | lo;
}

/*
 * Play one buffer a sample at a time.  Returns -EINTR if a signal arrives, so
 * a player can still be killed part way through a long block.
 */
static int FARPROC sb_pio_play(unsigned int off, unsigned int len)
{
    unsigned int ticks_per_sample;
    unsigned int prev, now, elapsed;
    unsigned int i;
    unsigned char sample;

    /* 1193182 / rate, computed once per block rather than per sample */
    {
        unsigned int rem = sb_rate;
        ticks_per_sample = (unsigned int)__divmod(1193182UL, &rem);
    }
    if (!ticks_per_sample)
        ticks_per_sample = 1;

    prev = pit_read();
    for (i = 0; i < len; i++) {
        sample = peekb((word_t)(off + i), sb_bounce_seg->base);
        if (dsp_cmd(DSP_DIRECT_DAC) < 0 || dsp_cmd(sample) < 0)
            return -EIO;
        /* wait out this sample's period on the 8253 */
        do {
            now = pit_read();
            elapsed = (prev - now) & 0xFFFF;
        } while (elapsed < ticks_per_sample);
        prev = (prev - ticks_per_sample) & 0xFFFF;
        if ((i & 0x3FF) == 0 && current->signal)
            return -EINTR;
    }
    return 0;
}

static int FARPROC sb_dsp_start(unsigned int len)
{
    unsigned int n = len - 1U;

    /*
     * The time constant is resent before EVERY block.  A real SB DSP latches
     * it across 0x14 transfers, and gating the resend behind a rate-change
     * flag measured fine on 86Box's SB Pro - but per-block resend is the only
     * behaviour ever validated on the real OPTi 82C929, and a distortion
     * report that arrived alongside the gating (later traced mainly to the
     * master level being at full scale) was reason enough to keep the proven
     * sequence.  Two dsp_cmd handshakes per 4096-byte block is a small price.
     */
    if (dsp_cmd(DSP_SET_RATE) < 0 || dsp_cmd(sb_timeconst) < 0)
        return -EIO;
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

static int FARPROC sb_start(unsigned int off, unsigned int len)
{
    unsigned int flags;
    unsigned int ticks;
    int ret;

    /*
     * PIO plays the whole block here and now, so sb_active is deliberately
     * left clear: there is no completion interrupt to wait for, and anything
     * that called sb_wait_complete afterwards would block forever.
     */
    if (sb_pio_mode)
        return sb_pio_play(off, len);

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

    (void)dsp_cmd(DSP_HALT_DMA);
    save_flags(flags);
    clr_irq();
    sb_idle();
    sb_fill = 0;
    restore_flags(flags);
}

/*
 * The mixer index and data ports need a real delay between them.  The OPTi
 * 82C929 reference driver writes the index, waits a millisecond through INT
 * 15h/86h, writes the data, then waits again (Knipperts, UNITS/SBPRO.PAS,
 * Mixer_Write); its companion SBFIX TSR spaces the same two writes two timer
 * ticks apart and says outright that "SB mixerchip needs a little delay".
 *
 * Issuing the two outb instructions back to back, which is what this driver did,
 * loses the write entirely on such a card.  That is why reads came back as
 * nonsense and why the output mode never changed: a mono stream was left playing
 * on a card still in stereo, which splits it across the channels and sounds like
 * one channel stuttering and the other badly aliased.
 *
 * inb from port 0x61 is the delay idiom already used for the DSP handshake; each
 * one is roughly a microsecond of ISA bus time.  Mixer writes only happen at
 * init and at open, so erring long here costs nothing.
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
 * Select mono output with the filter engaged.  The SB Pro is the model that
 * keeps this in the mixer; a plain SB has no mixer, and the SB16 takes its
 * channel count from the DSP command instead, so restrict this to DSP 3.xx as
 * Linux does with its MDL_SBPRO test.
 *
 * Linux does a read-modify-write here.  We write the register outright because
 * this is a two-bit register whose other bits we would only be preserving by
 * luck: on a MAD16 the mixer does not read back faithfully, and honouring bits
 * read from it would just write garbage back.
 */
/*
 * Deliberately does nothing.  The xt-elks driver that worked on this hardware
 * never wrote mixer register 0x0E in any shipped build - its whole stereo path
 * sits inside #ifdef CONFIG_SB_STEREO and that symbol is "not set" in its
 * config, so the only mixer registers it ever touched were 0x22 and 0x04.
 * Writing 0x0E is therefore not required to get mono out of this card, and the
 * card is left in whatever output mode it powered up in, exactly as before.
 *
 * Kept as an empty function rather than deleted so the ioctl call sites still
 * document where a card that genuinely needs the write would want it.
 */
/*
 * Mixer 0x0E bit 5 bypasses the SB Pro's output filter, a low pass around
 * 3.2kHz meant for low rate 8-bit playback.  Leaving it engaged above about
 * 8kHz throws away most of what the higher rate was for, so it is switched by
 * rate rather than left at the card's power-on default.  Bit 1 (stereo) stays
 * clear: this driver is mono only.
 */
#define SB_MIX_OUTMODE  0x0E
#define SB_OUT_FILT_OFF 0x20

static void FARPROC sb_set_output_mode(void)
{
    if (sb_has_mixer())
        sb_mixer_write(SB_MIX_OUTMODE,
            (unsigned char)((sb_rate > 8000U)? SB_OUT_FILT_OFF: 0x00));
    return;
}


/*
 * Cards can come out of reset with the voice or master level at zero, which
 * looks exactly like a driver that runs but produces no sound, so both are
 * always programmed rather than left at whatever reset gave us.
 *
 * The inputs are muted for the opposite reason.  This driver plays PCM and
 * nothing else, but the FM synthesiser, CD, line and microphone inputs are
 * summed into the same output, at whatever level the card powered up with -
 * 57% for FM on the board this was tested against.  Nothing initialises the FM
 * section, so that is somebody else's noise added to our output for no benefit.
 */
/*
 * A DSP 3.xx card packs these levels as two 3-bit fields at bits 7-5 and 3-1,
 * not two 4-bit fields; only the SB16 uses 4 bits.  That is also why this card
 * appears to OR 0x11 into every read: bits 4 and 0 are the don't-care bits of
 * the 3-bit packing, so they simply do not exist in the register file.
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
    sb_set_output_mode();
}

/*
 * Programmed again on every open rather than only at init.  A DSP reset makes
 * this card restore its own mixer defaults, which puts the FM input back to 60%
 * and undoes the mute, so anything that has touched the card since boot leaves
 * state we have to reassert rather than assume.
 */
static void INITPROC sb_mixer_init(void)
{
    sb_mixer_program();
}

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
    if (!sb_active)
        return;
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
 * Wait for the block in flight.
 *
 * The deadline is only a backstop against a card whose interrupt never arrives,
 * so it is deliberately generous: twice the block's own play time plus two
 * seconds.  A real card clocks the transfer from its own sample clock and
 * finishes in exactly the play time, but an emulated one is driven by the host
 * audio stack, which can stall DMA for a large fraction of a second while it
 * buffers or resamples.  Half a second of slack was measured to be too little
 * there.  Erring long costs nothing: it only changes how quickly a mis-set
 * irq= reports failure, and playback itself always ends on the interrupt.
 */
static int FARPROC sb_wait_complete(unsigned int len)
{
    jiff_t deadline = jiffies() + sb_play_ticks(len) * 2 + (2 * HZ) + 1;
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

#ifdef CONFIG_SB_MAD16
    /* something else may have moved the SB personality since we set it up */
    mad16_restore_profile();
#endif
    (void)dsp_cmd(DSP_SPEAKER_ON);
    sb_mixer_program();         /* a DSP reset may have undone all of it */
    sb_play_underruns = 0;
    sb_fill = 0;
    sb_rate_cache(sb_rate);
    sb_idle();
    sb_opened = 1;
    return 0;
}

static void FARPROC sb_release_impl(struct inode *inode, struct file *file)
{

    if (sb_active)
        (void)sb_wait_complete(sb_active_len);
    sb_halt();
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
    oss_int32_t val;
    unsigned int rate;
    int ret;


    switch (cmd) {
    case SNDCTL_DSP_RESET:
        sb_halt();
        return 0;

    case SNDCTL_DSP_SYNC:
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
            unsigned int ceiling = sb_pio_mode? SB_MAX_RATE_PIO: SB_MAX_RATE;

            rate = (val > (oss_int32_t)ceiling)? ceiling:
                   (val < (oss_int32_t)SB_MIN_RATE)? SB_MIN_RATE:
                   (unsigned int)val;
            sb_rate_cache(rate);
        }
        val = (oss_int32_t)sb_actual_rate();
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_SETFMT:
        ret = sb_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != (oss_int32_t)AFMT_QUERY && val != (oss_int32_t)AFMT_U8)
            return -EINVAL;
        val = (oss_int32_t)AFMT_U8;
        return sb_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETFMTS:
        val = (oss_int32_t)AFMT_U8;
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
        val = (oss_int32_t)SB_BOUNCE;
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

void INITPROC sb_dsp_init(void)
{
    addr_t phys;

    sb_base = CONFIG_SB_PORT;
    sb_irq_line = CONFIG_SB_IRQ;
    sb_dma = CONFIG_SB_DMA;

    if (sb_port_opt == 0)               /* sb=off */
        return;
    if (sb_port_opt > 0)
        sb_base = (unsigned int)sb_port_opt;
    if (sb_irq_opt >= 0)
        sb_irq_line = (unsigned char)sb_irq_opt;
    if (sb_dma_opt >= 0)
        sb_dma = (unsigned char)sb_dma_opt;
    /*
     * Same 8237 timing fix the disk needs.  Both drivers set it because either
     * may be the first to initialise, and writing the command register twice
     * with the same value is harmless - it is a single global setting for the
     * controller, not a per-channel one.
     */
    if (dma_extwrite_opt) {
        outb_p(DMA1_CMD_EXTWRITE, DMA1_CMD_REG);
        printk("sb: 8237 extended write enabled\n");
    }
    /* sb=port,irq,0 selects PIO playback: no 8237, no completion interrupt */
    if (sb_dma == 0) {
        sb_pio_mode = 1;
        sb_dma = 1;                 /* keep the cached port maths harmless */
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

#ifdef CONFIG_SB_MAD16
    if (mad16_opt > 0) {
        unsigned int port = (mad16_port_opt > 0)?
            (unsigned int)mad16_port_opt: sb_base;
        int irq = (mad16_irq_opt >= 0)? mad16_irq_opt: (int)sb_irq_line;
        int dma = (mad16_dma_opt >= 0)? mad16_dma_opt: (int)sb_dma;
        int rc = mad16_early_init(port, irq, dma);

        /*
         * Distinguish the two failures: a card with no jumpers depends on this
         * step, so "which of the two went wrong" is the whole diagnostic.
         */
        if (rc == 0)
            printk("sb: mad16 at 0x%x irq %d dma %d\n", port, irq, dma);
        else if (rc == -EINVAL)
            printk("sb: mad16 cannot route 0x%x irq %d dma %d, "
                   "allows port 0x220/0x240 irq 5/7 dma 1/3\n", port, irq, dma);
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
     * Stop the parallel port sharing our interrupt.  IRQ 7 is LPT1's line as
     * well as the card's, and the printer port powers up with its interrupt
     * enabled (control register 0x37A reads 0x3F on this machine).  A floating
     * or strobed ACK then fires the sound ISR at random points in a transfer,
     * which is audible as distortion; masking it measurably cleaned up DMA
     * playback on the PC1640.  0x0C leaves INIT and SELECT_IN in their idle
     * state with the interrupt bit clear.  ELKS' lp driver is polled and never
     * requests IRQ 7, so nothing else wants it.
     */
    if (sb_irq_line == 7)
        outb_p(LPT1_CTRL_IDLE, LPT1_CONTROL);

    if (request_irq((int)sb_irq_line, sb_interrupt, INT_GENERIC)) {
        printk("sb: irq %d busy\n", sb_irq_line);
        goto out_free;
    }
    if (dsp_cmd(DSP_SPEAKER_ON) < 0) {
        printk("sb: dsp not responding\n");
        goto out_irq;
    }
    sb_mixer_init();

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

#endif /* CONFIG_CHAR_DEV_DSP */
