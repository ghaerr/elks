/*
 * AD1848 / Windows Sound System /dev/dsp driver
 *
 * Drives the codec directly instead of going through the Sound Blaster
 * personality an OPTi 82C929 can emulate.  The split follows Linux OSS, where
 * ad1848.c owns the codec and mad16.c only does the chip-specific bring-up:
 * everything here talks to the AD1848, and dsp-mad16.c puts the 82C929 into
 * Windows Sound mode and decodes the codec window.
 *
 * Why not the Sound Blaster path.  On a MAD16 the SB DSP is emulation logic
 * that programs this same codec through a shadow register, so the driver never
 * sees or controls the format the codec actually runs in.  Two consequences are
 * documented rather than incidental: the SB time constant is not a programmable
 * clock but is mapped to whichever crystal divider is nearest (82C929 datasheet
 * page 9, "FMAP: In Normal mode, frequency is mapped to the nearest frequency
 * using both crystals of the CODEC"), and the codec window is closed while SB
 * mode is active unless MC5 SPACCESS is set, which on the board this was
 * written for stops the SB personality answering at all.  Here the format,
 * channel count and rate are single fields this driver writes and can read back.
 *
 * Playback is mono unsigned 8-bit PCM, one DMA block in flight, completion from
 * the codec interrupt.  The bounce buffer holds two blocks so the next one is
 * copied while the current one plays.
 *
 * The register access idiom is the vendor driver's (Knipperts, AD1848.PAS
 * WriteCODECReg/ReadCODECReg) and is not obvious: the index register's upper
 * nibble holds INIT, MCE and TRD, so a write must preserve it rather than
 * assign the index outright, and the data port is written and read three times
 * because the part needs it.  Getting this wrong leaves MCE clear, and the
 * format register silently refuses writes when MCE is clear.
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_DSP_AD1848

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
 * Bus recovery on every access, as the xt-elks driver that worked on this
 * machine had through CONFIG_XT_SLOW_ISA_IO.  Back-to-back ISA cycles are
 * mis-latched by slow parts on an 8 MHz machine, and an emulator does not model
 * it, so this reads clean under emulation and fails on the bench.
 */
#undef  inb
#undef  outb
#define inb     inb_p
#define outb    outb_p

/*
 * The codec occupies four ports at WSS base + 4.  WSS base + 0 is not a codec
 * register: it is the board's write-only IRQ/DMA configuration register, bits
 * 5:3 selecting the IRQ and bits 2:0 the DRQ.  Nothing else on a WSS/MSS card
 * says which DRQ line the codec should assert, so it must be written before
 * any DMA transfer or the card keeps requesting on whatever channel it powered
 * up with (86Box's WSS powers up on DMA 3) while the 8237 is armed on another.
 * Reads return the Version register instead, so the routing cannot be read
 * back; the vendor tool mirrors that register's bit 7 into the byte it writes
 * (OPTI929.PAS, SetupWSSPort).
 */
#define AD_WSS_CFG      0           /* offset from the WSS base, not ad_base */
#define AD_CFG_IRQ7     0x08
#define AD_CFG_IRQ9     0x10
#define AD_CFG_IRQ10    0x18
#define AD_CFG_IRQ11    0x20
#define AD_CFG_DRQ0     0x01
#define AD_CFG_DRQ1     0x02
#define AD_CFG_DRQ3     0x03

#define AD_INDEX        0           /* index address, upper nibble = INIT/MCE/TRD */
#define AD_DATA         1           /* indexed data */
#define AD_STATUS       2           /* status, write anything to clear INT */
#define AD_PIO          3           /* programmed I/O data */

#define AD_INIT         0x80        /* index reads 0x80 while initialising */
#define AD_MCE          0x40        /* mode change enable */

/* Indirect registers used here */
#define AD_I_LEFT_DAC   6
#define AD_I_RIGHT_DAC  7
#define AD_I_FORMAT     8           /* clock and data format */
#define AD_I_IFACE      9           /* interface configuration */
#define AD_I_PIN        10
#define AD_I_TEST       11          /* ACI lives here */
#define AD_I_COUNT_HI   14
#define AD_I_COUNT_LO   15

/* I8 clock and data format */
#define AD_FMT_STEREO   0x10        /* clear = mono, plays both channels */
#define AD_FMT_COMPAND  0x20        /* clear = linear PCM */
#define AD_FMT_16BIT    0x40        /* with linear PCM: clear = 8-bit unsigned */
#define AD_FMT_XTAL2    0x01        /* clock source: clear = 24.576, set = 16.9344 */

/* I9 interface configuration */
#define AD_IFACE_PEN    0x01        /* playback enable, writable without MCE */
#define AD_IFACE_CEN    0x02
#define AD_IFACE_SDC    0x04        /* single DMA channel */
#define AD_IFACE_ACAL   0x08
#define AD_IFACE_PPIO   0x40        /* playback via programmed I/O, not DMA */

#define AD_PIN_IEN      0x02        /* I10: interrupt enable */
#define AD_TEST_ACI     0x20        /* I11: autocalibrate in progress */
#define AD_STAT_INT     0x01
#define AD_STAT_PRDY    0x02        /* playback FIFO wants a byte (PIO mode) */
#define AD_DAC_MUTE     0x80        /* I6/I7 bit 7 */

/*
 * Output level as attenuation in 1.5 dB steps, 0 being 0 dB.  The Sound Blaster
 * driver settled on about 70% of full scale on this hardware, which is roughly
 * 8 steps down.  These registers power up muted.
 */
#define AD_DAC_ATTEN    8

#define AD_BLOCK        CONFIG_SB_BOUNCE
#define AD_BUFFER       (2 * AD_BLOCK)

/*
 * Rates the codec can actually produce.  There is no programmable divisor: the
 * three CFS bits pick a divide factor and CSS picks one of two crystals, so
 * these fourteen values are the whole set (AD1848 datasheet page 17).  Anything
 * else has to be resampled in user space, which is why SNDCTL_DSP_SPEED reports
 * back the rate that was really selected.
 */
struct ad_rate {
    unsigned int rate;
    unsigned char fmt;              /* CFS in bits 3:1, CSS in bit 0 */
};

static struct ad_rate ad_rates[] = {
    {  5512, (0 << 1) | 1 },
    {  6620, (7 << 1) | 1 },
    {  8000, (0 << 1) | 0 },
    {  9600, (7 << 1) | 0 },
    { 11025, (1 << 1) | 1 },
    { 16000, (1 << 1) | 0 },
    { 18900, (2 << 1) | 1 },
    { 22050, (3 << 1) | 1 },
    { 27420, (2 << 1) | 0 },
    { 32000, (3 << 1) | 0 },
    { 33075, (6 << 1) | 1 },
    { 37800, (4 << 1) | 1 },
    { 44100, (5 << 1) | 1 },
    { 48000, (6 << 1) | 0 },
};
#define AD_NRATES   (sizeof(ad_rates) / sizeof(ad_rates[0]))

/* set up by dsp-mad16.c when it puts the chip into Windows Sound mode */
extern unsigned int mad16_wss_base;
extern unsigned char mad16_wss_irq;
extern unsigned char mad16_wss_dma;

static unsigned int ad_base;        /* codec window, = wss base + 4 */
static unsigned char ad_irq;
static unsigned char ad_dma;
static unsigned char ad_present;
/*
 * Transport selection.  DMA is correct almost everywhere and is what the
 * emulated card needs; but on the PC1640 the codec's WSS-mode request never
 * reaches the 8237 (the same wire works in Sound Blaster mode, so it is the
 * board's WSS routing, not the machine).  Rather than configure this, the
 * first open runs one short muted DMA block and reads the 8237 count back:
 * if it moved, DMA it is; if not, playback falls back to the codec's own
 * PIO port, which its crystal still paces, so the rate stays exact.
 */
static unsigned char ad_pio_mode;       /* set by ad_dma_probe on dead DMA */
static unsigned char ad_dma_probed;
static unsigned char ad_pio_running;    /* PEN held up across PIO blocks */
static unsigned char ad_opened;

static unsigned int ad_rate = 8000;
static unsigned char ad_rate_fmt = (0 << 1) | 0;

static unsigned int ad_dma_addr_port;
static unsigned int ad_dma_page_reg;
static unsigned int ad_dma_count_port;
static unsigned char ad_dma_mask;

static segment_s *ad_buf_seg;
static unsigned int ad_buf_off;
static unsigned char ad_buf_page;
static unsigned char ad_fill;       /* half to copy into next */

static unsigned char ad_active;
static unsigned int ad_active_len;
static volatile unsigned char ad_done;
static struct wait_queue ad_wait;

static struct audio_errinfo ad_errinfo;
static oss_int32_t ad_underruns;
static oss_int32_t ad_polled;       /* blocks completed without an IRQ */
static oss_int32_t ad_blocks;       /* blocks started */

/*
 * The index register reads 0x80 for as long as the part is initialising, and
 * every other access has to wait that out first.
 */
static int FARPROC ad_wait_ready(void)
{
    unsigned int timeout = 60000;

    while (timeout-- != 0) {
        if (inb(ad_base + AD_INDEX) != AD_INIT)
            return 0;
    }
    return -EIO;
}

/*
 * Preserve the index register's upper nibble: it carries INIT, MCE and TRD, and
 * assigning the index outright would drop MCE, at which point a write to the
 * format register is silently discarded.  The triple data access is the vendor
 * driver's, which marks the read side "Errata for AD1848".
 */
static void FARPROC ad_write(unsigned char reg, unsigned char value)
{
    unsigned int flags;
    unsigned char old;

    if (ad_wait_ready() < 0)
        return;
    save_flags(flags);
    clr_irq();
    old = inb(ad_base + AD_INDEX);
    outb((unsigned char)((old & 0xF0) | (reg & 0x0F)), ad_base + AD_INDEX);
    outb(value, ad_base + AD_DATA);
    outb(value, ad_base + AD_DATA);
    outb(value, ad_base + AD_DATA);
    (void)inb(ad_base + AD_DATA);
    outb(old, ad_base + AD_INDEX);
    restore_flags(flags);
}

static unsigned char FARPROC ad_read(unsigned char reg)
{
    unsigned int flags;
    unsigned char old, v;

    if (ad_wait_ready() < 0)
        return 0xFF;
    save_flags(flags);
    clr_irq();
    old = inb(ad_base + AD_INDEX);
    outb((unsigned char)((old & 0xF0) | (reg & 0x0F)), ad_base + AD_INDEX);
    v = inb(ad_base + AD_DATA);
    v = inb(ad_base + AD_DATA);
    v = inb(ad_base + AD_DATA);
    outb(old, ad_base + AD_INDEX);
    restore_flags(flags);
    return v;
}

static void FARPROC ad_mce_on(void)
{
    unsigned char v;

    (void)ad_wait_ready();
    v = inb(ad_base + AD_INDEX);
    outb((unsigned char)(v | AD_MCE), ad_base + AD_INDEX);
}

/*
 * Clearing MCE starts an autocalibration when ACAL is set.  The part signals it
 * by raising ACI in I11 and then dropping it again, and the DAC output is muted
 * throughout, so playback must not be started until it has finished.
 */
/*
 * Release MCE without waiting for autocalibration.
 *
 * The ACI wait below only makes sense when ACAL is set in I9, because that is
 * what starts a calibration.  Calling the waiting form on the per-block
 * playback path - where ACAL is deliberately clear - spins both 60000 iteration
 * loops in full, each iteration an indexed codec read of several bus cycles.
 * That is roughly a third of a second of dead polling per 4KB block, which at
 * 16kHz starves the machine badly enough to look like a hang.
 */
static void FARPROC ad_mce_off_nowait(void)
{
    unsigned char v;

    (void)ad_wait_ready();
    v = inb(ad_base + AD_INDEX);
    outb((unsigned char)(v & ~AD_MCE), ad_base + AD_INDEX);
}

static void FARPROC ad_mce_off(void)
{
    unsigned char v;
    unsigned int timeout;

    (void)ad_wait_ready();
    v = inb(ad_base + AD_INDEX);
    outb((unsigned char)(v & ~AD_MCE), ad_base + AD_INDEX);

    timeout = 60000;
    while (timeout-- != 0)               /* wait for ACI to rise */
        if ((ad_read(AD_I_TEST) & AD_TEST_ACI) != 0)
            break;
    timeout = 60000;
    while (timeout-- != 0)               /* and fall again */
        if ((ad_read(AD_I_TEST) & AD_TEST_ACI) == 0)
            break;
}

/* Nearest achievable rate, since the divider table is all there is. */
static unsigned char FARPROC ad_rate_select(unsigned int want)
{
    unsigned int i, best = 0, bestd = 0xFFFF, d;

    for (i = 0; i < AD_NRATES; i++) {
        d = (ad_rates[i].rate > want)? ad_rates[i].rate - want:
                                       want - ad_rates[i].rate;
        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    ad_rate = ad_rates[best].rate;
    return ad_rates[best].fmt;
}

/*
 * Format, channel count and rate are one register.  It only accepts writes
 * while MCE is set, and the vendor driver reads the data port twice afterwards
 * before leaving MCE, which it attributes to the GUS MAX SDK.
 */
static void FARPROC ad_set_format(void)
{
    ad_mce_on();
    /* linear PCM, 8-bit unsigned, mono: only the rate bits are set */
    ad_write(AD_I_FORMAT, ad_rate_fmt);
    (void)ad_read(AD_I_FORMAT);
    (void)ad_read(AD_I_FORMAT);
    ad_mce_off();
}

static void FARPROC ad_dma_cache(void)
{
    if (ad_dma == 1) {
        ad_dma_addr_port = DMA_ADDR_1;
        ad_dma_count_port = DMA_CNT_1;
        ad_dma_page_reg = DMA_PAGE_1;
    } else {
        ad_dma_addr_port = DMA_ADDR_3;
        ad_dma_count_port = DMA_CNT_3;
        ad_dma_page_reg = DMA_PAGE_3;
    }
    ad_dma_mask = ad_dma;
}

/* Caller holds interrupts off: the 8237 address flip-flop is shared state. */
static void FARPROC ad_dma_program(unsigned int off, unsigned int len)
{
    union {
        unsigned int word;
        unsigned char byte[2];
    } addr, count;

    addr.word = ad_buf_off + off;
    count.word = len - 1U;

    outb((unsigned char)(ad_dma_mask | 4), DMA1_MASK_REG);
    outb(0, DMA1_CLEAR_FF_REG);
    outb((unsigned char)(DMA_MODE_WRITE | ad_dma), DMA1_MODE_REG);
    outb(addr.byte[0], ad_dma_addr_port);
    outb(addr.byte[1], ad_dma_addr_port);
    outb(ad_buf_page, ad_dma_page_reg);
    outb(count.byte[0], ad_dma_count_port);
    outb(count.byte[1], ad_dma_count_port);
    outb(ad_dma_mask, DMA1_MASK_REG);
}

static void FARPROC ad_stop(void)
{
    unsigned char v;

    v = ad_read(AD_I_IFACE);
    /* Keep PPIO, drop PEN: the DOS driver stops a transfer by writing I9 with
     * PEN clear, and the PIO path still needs its mode bit left alone. */
    ad_write(AD_I_IFACE, (unsigned char)(v & ~(AD_IFACE_PEN | AD_IFACE_SDC)));
    outb((unsigned char)(ad_dma_mask | 4), DMA1_MASK_REG);
    outb(0, ad_base + AD_STATUS);       /* any write clears INT */
    ad_pio_running = 0;
    ad_active = 0;
    ad_active_len = 0;
    ad_done = 0;
}

/*
 * The codec counts samples, not bytes, and interrupts when the counter
 * underflows, so the base count is one less than the block.
 */
/*
 * PIO playback, selected with ad1848=pio.
 *
 * With PPIO set in I9 the codec stops requesting DMA and instead raises PRDY in
 * the status register whenever its FIFO wants another byte, which the CPU then
 * writes to the PIO data port.  That makes this the best of both worlds on this
 * machine: no 8237 - which has proved unreliable here for both the sound card
 * and the disk - yet the pacing still comes from the codec's own crystal, so
 * the rate is exact rather than depending on a software delay loop.
 */
static int FARPROC ad_pio_play(unsigned int off, unsigned int len)
{
    unsigned int i, spins;
    unsigned char sample;

    /*
     * Playback is started once and PEN then stays up across blocks.  The old
     * per-block MCE on/off pair muted the DAC for about 128 sample periods on
     * each transition - a click and a hole in the sound every 4096 bytes.
     * With PEN held, a late block just makes the codec repeat its last sample
     * until the next byte arrives, which is far less audible than a mute.
     * The sample counter free-runs (it reloads itself on underflow) and no
     * interrupt is wanted: PRDY alone paces the feed.
     */
    if (!ad_pio_running) {
        outb_p(0, ad_base + AD_STATUS);
        ad_write(AD_I_COUNT_LO, 0xFF);
        ad_write(AD_I_COUNT_HI, 0xFF);
        ad_write(AD_I_PIN, 0);
        ad_mce_on();
        ad_write(AD_I_IFACE,
            (unsigned char)(AD_IFACE_SDC | AD_IFACE_PPIO | AD_IFACE_PEN));
        ad_mce_off_nowait();
        ad_pio_running = 1;
    }

    for (i = 0; i < len; i++) {
        spins = 0;
        while ((inb_p(ad_base + AD_STATUS) & AD_STAT_PRDY) == 0) {
            if (++spins == 0)           /* wrapped: the codec stopped asking */
                return -EIO;
        }
        sample = peekb((word_t)(off + i), ad_buf_seg->base);
        outb_p(sample, ad_base + AD_PIO);
        if ((i & 0x3FF) == 0 && current->signal) {
            /* PEN and CEN are the two bits writable without MCE, so this
             * stops playback bare; PPIO keeps its programmed value. */
            ad_write(AD_I_IFACE, AD_IFACE_SDC);
            ad_pio_running = 0;
            return -EINTR;
        }
    }
    return 0;
}

static int FARPROC ad_start(unsigned int off, unsigned int len)
{
    /*
     * PIO plays the whole block here, so ad_active stays clear: there is no
     * completion interrupt to wait for and ad_wait_done would block forever.
     */
    if (ad_pio_mode)
        return ad_pio_play(off, len);

    unsigned int flags;
    unsigned int n = len - 1U;

    save_flags(flags);
    clr_irq();
    ad_done = 0;
    ad_dma_program(off, len);
    ad_blocks++;
    ad_active = 1;
    ad_active_len = len;
    restore_flags(flags);

    outb(0, ad_base + AD_STATUS);       /* clear any stale INT */
    ad_write(AD_I_COUNT_LO, (unsigned char)(n & 0xFF));
    ad_write(AD_I_COUNT_HI, (unsigned char)(n >> 8));
    ad_write(AD_I_PIN, AD_PIN_IEN);
    /*
     * SDC|PEN matches the DOS driver's power-on table for a single-channel
     * card (AD1848.PAS init value $0c).  SDC only diverts capture onto the
     * playback channel, so for playback-only use either setting plays; set
     * matches the vendor table.  I11 is read-only status - there is nothing
     * to clear there, the only sticky bit anywhere is INT, cleared by writing
     * the status port, which ad_start() already does above.
     */
    ad_write(AD_I_IFACE, (unsigned char)(AD_IFACE_SDC | AD_IFACE_PEN));
    return 0;
}

/*
 * Must not be FARPROC: request_irq stores a near pointer and the trampoline
 * calls this with KERNEL_CS.  The PIC end-of-interrupt is issued by the irqit
 * wrapper, so do not touch the PIC here.
 */
static void ad_interrupt(int irq, struct pt_regs *regs)
{
    (void)irq;
    (void)regs;

    if ((inb(ad_base + AD_STATUS) & AD_STAT_INT) == 0)
        return;                         /* not ours */
    outb(0, ad_base + AD_STATUS);       /* any write clears it */
    if (!ad_active)
        return;
    ad_done = 1;
    wake_up(&ad_wait);
}

static int FARPROC ad_wait_done(unsigned int len)
{
    jiff_t deadline;
    unsigned int rem = ad_rate;
    unsigned int ticks;

    ticks = (unsigned int)__divmod((unsigned long)len * HZ, &rem);
    deadline = (jiff_t)jiffies() + (jiff_t)ticks * 2 + (2 * HZ) + 1;

    for (;;) {
        prepare_to_wait_interruptible(&ad_wait);
        /*
         * The codec's own INT bit is checked as well as ad_done, because the
         * interrupt cannot be relied on here.  The WSS configuration register
         * can only encode IRQ 7, 9, 10 or 11, and this is an XT class machine
         * with one 8259 - 9, 10 and 11 do not exist, so 7 is forced, which is
         * both LPT1 and the PIC's spurious vector.  Polling the status register
         * makes a block complete whether or not the interrupt ever arrives; the
         * handler still short circuits the wait when it does fire.
         */
        if (ad_done || (inb(ad_base + AD_STATUS) & AD_STAT_INT)) {
            if (!ad_done)
                ad_polled++;            /* the interrupt did not arrive */
            outb(0, ad_base + AD_STATUS);
            finish_wait(&ad_wait);
            break;
        }
        if (current->signal) {
            finish_wait(&ad_wait);
            ad_stop();
            return -EINTR;
        }
        if (time_after(jiffies(), deadline)) {
            finish_wait(&ad_wait);
            ad_underruns++;
            ad_stop();
            return -EIO;
        }
        current->timeout = (jiff_t)jiffies() + 1;
        do_wait();
        current->timeout = 0;
        finish_wait(&ad_wait);
    }

    outb((unsigned char)(ad_dma_mask | 4), DMA1_MASK_REG);
    ad_active = 0;
    return 0;
}

/* Copy into the idle half first so it overlaps playback of the other one. */
static int FARPROC ad_queue(char *buf, unsigned int len)
{
    unsigned int off;
    int ret;

    if (len > AD_BLOCK)
        return -EINVAL;
    if (verify_area(VERIFY_READ, buf, len) != 0)
        return -EFAULT;

    off = ad_fill? AD_BLOCK: 0;
    fmemcpyb((void *)off, ad_buf_seg->base, buf, current->t_regs.ds, len);

    if (ad_active) {
        ret = ad_wait_done(ad_active_len);
        if (ret < 0)
            return ret;
    }
    ret = ad_start(off, len);
    if (ret < 0)
        return ret;
    ad_fill ^= 1;
    return 0;
}

static void FARPROC ad_dma_probe(void)
{
    unsigned int flags, count, i;

    ad_dma_probed = 1;
    /* silence, and the DAC is still muted here, so the probe is inaudible */
    fmemsetb((void *)0, ad_buf_seg->base, 0x80, 256);

    save_flags(flags);
    clr_irq();
    ad_dma_program(0, 256);
    restore_flags(flags);

    outb(0, ad_base + AD_STATUS);
    ad_write(AD_I_COUNT_LO, 255);
    ad_write(AD_I_COUNT_HI, 0);
    ad_write(AD_I_PIN, 0);              /* no interrupt for the probe */
    ad_write(AD_I_IFACE, (unsigned char)(AD_IFACE_SDC | AD_IFACE_PEN));
    /* ~50ms of bus cycles: hundreds of sample periods at any rate */
    for (i = 0; i < 40000U; i++)
        inb(ad_base + AD_STATUS);
    ad_write(AD_I_IFACE, AD_IFACE_SDC);
    outb(0, ad_base + AD_STATUS);

    save_flags(flags);
    clr_irq();
    outb((unsigned char)(ad_dma_mask | 4), DMA1_MASK_REG);
    outb(0, DMA1_CLEAR_FF_REG);
    count = inb(ad_dma_count_port);
    count |= (unsigned int)inb(ad_dma_count_port) << 8;
    restore_flags(flags);

    if (count == 255) {                 /* untouched: nothing ever transferred */
        ad_pio_mode = 1;
        printk("ad1848: dma %d dead, using pio\n", ad_dma);
    } else
        printk("ad1848: dma %d ok\n", ad_dma);
}

static int FARPROC ad_open_impl(struct inode *inode, struct file *file)
{
    (void)file;

    if (!ad_present || !ad_buf_seg)
        return -ENODEV;
    if (MINOR(inode->i_rdev) != 0)
        return -ENODEV;
    if (ad_opened)
        return -EBUSY;

    ad_underruns = 0;
    ad_fill = 0;
    ad_set_format();
    if (!ad_dma_probed && !ad_pio_mode)
        ad_dma_probe();
    ad_write(AD_I_LEFT_DAC, AD_DAC_ATTEN);      /* unmuted, bit 7 clear */
    ad_write(AD_I_RIGHT_DAC, AD_DAC_ATTEN);
    ad_opened = 1;
    return 0;
}

static void FARPROC ad_release_impl(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;

    if (ad_active)
        (void)ad_wait_done(ad_active_len);
    ad_stop();
    ad_write(AD_I_LEFT_DAC, AD_DAC_MUTE);
    ad_write(AD_I_RIGHT_DAC, AD_DAC_MUTE);
    /* Says plainly whether irq %d is live, rather than leaving it to be guessed
     * from the sound: any nonzero polled count means blocks finished without
     * the interrupt arriving. */
    if (!ad_pio_mode)
        printk("ad1848: %ld blocks, %ld no-irq, %ld underruns\n",
               (long)ad_blocks, (long)ad_polled, (long)ad_underruns);
    ad_opened = 0;
}

static size_t FARPROC ad_read_impl(struct inode *inode, struct file *file,
                                   char *buf, size_t count)
{
    (void)inode; (void)file; (void)buf; (void)count;
    return 0;                           /* playback only */
}

static size_t FARPROC ad_write_impl(struct inode *inode, struct file *file,
                                    char *buf, size_t count)
{
    size_t done = 0;
    unsigned int chunk;
    int ret;

    (void)inode;

    if (!(file->f_mode & FMODE_WRITE))
        return -EINVAL;

    while (done < count) {
        chunk = (unsigned int)(count - done);
        if (chunk > AD_BLOCK)
            chunk = AD_BLOCK;
        ret = ad_queue(buf + done, chunk);
        if (ret < 0)
            return done? done: (size_t)ret;
        done += chunk;
    }
    return done;
}

static int FARPROC ad_get_arg(char *arg, void *dst, size_t len)
{
    if (!arg)
        return -EINVAL;
    return verified_memcpy_fromfs(dst, arg, len);
}

static int FARPROC ad_put_arg(char *arg, void *src, size_t len)
{
    if (!arg)
        return -EINVAL;
    return verified_memcpy_tofs(arg, src, len);
}

static int FARPROC ad_ioctl_impl(struct inode *inode, struct file *file,
                                 unsigned int cmd, unsigned int arg_in)
{
    char *arg = (char *)arg_in;
    oss_int32_t val;
    int ret;

    (void)inode;
    (void)file;

    switch (cmd) {
    case SNDCTL_DSP_RESET:
        ad_stop();
        ad_fill = 0;
        return 0;

    case SNDCTL_DSP_SYNC:
        if (ad_active) {
            ret = ad_wait_done(ad_active_len);
            if (ret < 0)
                return ret;
        }
        ad_stop();
        ad_fill = 0;
        return 0;

    case SNDCTL_DSP_POST:
        return 0;

    case SNDCTL_DSP_SPEED:
        ret = ad_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val > 0) {
            if (ad_pio_running)
                ad_stop();
            ad_rate_fmt = ad_rate_select((unsigned int)val);
            /*
             * Program it now.  Applying the format only in open() left the
             * part at whatever rate the previous open selected, because the
             * rate is always set after the open, never before it - so a 16kHz
             * stream played out of a codec still clocked at 8000Hz, at half
             * speed.
             */
            if (!ad_active)
                ad_set_format();
        }
        val = (oss_int32_t)ad_rate;     /* what was really selected */
        return ad_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_SETFMT:
        ret = ad_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != (oss_int32_t)AFMT_QUERY && val != (oss_int32_t)AFMT_U8)
            return -EINVAL;
        val = (oss_int32_t)AFMT_U8;
        return ad_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETFMTS:
        val = (oss_int32_t)AFMT_U8;
        return ad_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_CHANNELS:
        ret = ad_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != 0 && val != 1)
            return -EINVAL;
        val = 1;
        return ad_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_STEREO:
        ret = ad_get_arg(arg, &val, sizeof(val));
        if (ret)
            return ret;
        if (val != 0)
            return -EINVAL;
        val = 0;
        return ad_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETBLKSIZE:
        val = (oss_int32_t)AD_BLOCK;
        return ad_put_arg(arg, &val, sizeof(val));

    case SNDCTL_DSP_GETERROR:
        memset(&ad_errinfo, 0, sizeof(ad_errinfo));
        ad_errinfo.play_underruns = ad_underruns;
        ad_underruns = 0;
        return ad_put_arg(arg, &ad_errinfo, sizeof(ad_errinfo));

    default:
        return -EINVAL;
    }
}

/*
 * The file_operations slots have to be near-callable, so these thin wrappers
 * are what keeps the body of the driver in far text.
 */
static int ad_fop_open(struct inode *inode, struct file *file)
{
    return ad_open_impl(inode, file);
}

static void ad_fop_release(struct inode *inode, struct file *file)
{
    ad_release_impl(inode, file);
}

static size_t ad_fop_read(struct inode *inode, struct file *file, char *buf,
                      size_t count)
{
    return ad_read_impl(inode, file, buf, count);
}

static size_t ad_fop_write(struct inode *inode, struct file *file, char *buf,
                       size_t count)
{
    return ad_write_impl(inode, file, buf, count);
}

static int ad_fop_ioctl(struct inode *inode, struct file *file, int cmd, char *arg)
{
    return ad_ioctl_impl(inode, file, (unsigned int)cmd, (unsigned int)arg);
}

static struct file_operations ad_dsp_fops = {
    NULL,                       /* lseek */
    ad_fop_read,
    ad_fop_write,
    NULL,                       /* readdir */
    NULL,                       /* select */
    ad_fop_ioctl,
    ad_fop_open,
    ad_fop_release
};

/*
 * A single 8237 transfer cannot step across a physical 64K page, and the whole
 * two-block buffer is checked so that adding a half-buffer offset to the cached
 * low word can never carry into the page byte.
 */
#define AD_ALLOC_TRIES  4

static int INITPROC ad_dma_unusable(addr_t phys)
{
    if (phys + (addr_t)AD_BUFFER > 0x100000UL)
        return 1;
    return ((phys & 0xffffUL) + (addr_t)AD_BUFFER - 1) > 0xffffUL;
}

static addr_t INITPROC ad_alloc_buffer(void)
{
    segment_s *reject[AD_ALLOC_TRIES];
    segment_s *seg;
    addr_t phys = 0;
    int nreject = 0;
    int i;

    for (i = 0; i < AD_ALLOC_TRIES; i++) {
        seg = seg_alloc((segext_t)((AD_BUFFER + 15) >> 4),
                        SEG_FLAG_EXTBUF | SEG_FLAG_ALIGN1K);
        if (!seg)
            break;
        phys = LINADDR(seg->base, 0);
        if (!ad_dma_unusable(phys)) {
            ad_buf_seg = seg;
            break;
        }
        reject[nreject++] = seg;
    }
    while (nreject-- > 0)
        seg_put(reject[nreject]);

    return ad_buf_seg? phys: 0;
}

/*
 * Present if the index register stops reading back as the initialising value
 * and a written index reads back.  Done after the controller has been put into
 * Windows Sound mode, so the codec window is decoded.
 */
static int INITPROC ad_detect(void)
{
    unsigned char old;

    if (inb(ad_base + AD_INDEX) == 0xFF)
        return 0;
    if (ad_wait_ready() < 0)
        return 0;

    old = inb(ad_base + AD_INDEX);
    outb((unsigned char)((old & 0xF0) | 0x0A), ad_base + AD_INDEX);
    if ((inb(ad_base + AD_INDEX) & 0x0F) != 0x0A) {
        outb(old, ad_base + AD_INDEX);
        return 0;
    }
    outb(old, ad_base + AD_INDEX);
    return 1;
}

/*
 * Route the codec's DRQ and IRQ.  mad16_wss_init() writes this byte too, but
 * only after it has found an 82c929, and a plain WSS card has no 82c929 to
 * find - which left the card on its power-on channel while ad_dma_program()
 * armed CONFIG_AD1848_DMA, so the codec ran, counted samples and interrupted
 * on schedule while not one byte ever moved.  Writing it here covers both
 * cases; on an OPTi board it repeats the identical byte.
 */
static void INITPROC ad_wss_config(unsigned int wss_base, int irq, int dma)
{
    unsigned char cfg;

    switch (irq) {
    case 7:  cfg = AD_CFG_IRQ7;  break;
    case 9:  cfg = AD_CFG_IRQ9;  break;
    case 10: cfg = AD_CFG_IRQ10; break;
    case 11: cfg = AD_CFG_IRQ11; break;
    default:                            /* notably not 5: WSS cannot encode it */
        printk("ad1848: irq %d has no wss encoding\n", irq);
        return;
    }
    switch (dma) {
    case 0: cfg |= AD_CFG_DRQ0; break;
    case 1: cfg |= AD_CFG_DRQ1; break;
    case 3: cfg |= AD_CFG_DRQ3; break;
    default:
        printk("ad1848: dma %d has no wss encoding\n", dma);
        return;
    }
    if (inb(wss_base + AD_WSS_CFG) & 0x80)      /* mirror Version bit 7 */
        cfg |= 0x80;
    outb(cfg, wss_base + AD_WSS_CFG);
    printk("ad1848: wss cfg 0x%x, irq %d drq %d\n", cfg, irq, dma);
}

void INITPROC ad1848_dsp_init(void)
{
    addr_t phys;

    /*
     * The OPTi front end is not required.  A plain Windows Sound System or
     * bare AD1848 card decodes the codec window itself and needs no bring-up,
     * so a failed detect is only fatal if no codec answers either - which
     * ad_detect() below decides.  This also makes the driver testable against
     * an emulated WSS card, where there is no 82c929 to find.
     */
    if (mad16_wss_init(CONFIG_AD1848_WSS_BASE, CONFIG_AD1848_IRQ,
                       CONFIG_AD1848_DMA) < 0)
        printk("ad1848: no 82c929, assuming plain wss\n");
    /*
     * Always after mad16_wss_init(): on an OPTi board MC1 must first put the
     * chip in WSS mode and select the codec window before this port decodes.
     */
    ad_wss_config((unsigned int)CONFIG_AD1848_WSS_BASE, CONFIG_AD1848_IRQ,
                  CONFIG_AD1848_DMA);
    ad_base = (unsigned int)CONFIG_AD1848_WSS_BASE + 4;
    ad_irq = (unsigned char)CONFIG_AD1848_IRQ;
    ad_dma = (unsigned char)CONFIG_AD1848_DMA;

    if (ad_dma != 1 && ad_dma != 3) {
        printk("ad1848: dma %d not 1 or 3\n", ad_dma);
        return;
    }
#ifdef CONFIG_BLK_DEV_MFMHD
    if (ad_dma == 3) {
        printk("ad1848: dma 3 belongs to the hard disk controller\n");
        return;
    }
#endif
    if (!ad_detect()) {
        printk("ad1848: no codec at 0x%x\n", ad_base);
        return;
    }
    ad_dma_cache();

    phys = ad_alloc_buffer();
    if (!phys) {
        printk("ad1848: no dma buffer below 1M\n");
        return;
    }
    ad_buf_off = (unsigned int)phys;
    ad_buf_page = (unsigned char)(phys >> 16);

    if (request_irq((int)ad_irq, ad_interrupt, INT_GENERIC)) {
        printk("ad1848: irq %d busy\n", ad_irq);
        goto out_free;
    }

    /* mono, 8-bit unsigned, and let the part autocalibrate once */
    ad_rate_fmt = ad_rate_select(ad_rate);
    ad_mce_on();
    ad_write(AD_I_FORMAT, ad_rate_fmt);
    (void)ad_read(AD_I_FORMAT);
    (void)ad_read(AD_I_FORMAT);
    ad_write(AD_I_IFACE, (unsigned char)(AD_IFACE_SDC | AD_IFACE_ACAL));
    ad_mce_off();

    ad_write(AD_I_LEFT_DAC, AD_DAC_MUTE);
    ad_write(AD_I_RIGHT_DAC, AD_DAC_MUTE);

    if (register_chrdev(DSP_MAJOR, "dsp", &ad_dsp_fops)) {
        printk("ad1848: unable to register\n");
        goto out_irq;
    }
    ad_present = 1;
    printk("ad1848: codec at 0x%x irq %d %s, %u byte buffer, %u Hz\n",
        ad_base, ad_irq, ad_pio_mode? "pio": "dma", (unsigned int)AD_BLOCK,
        ad_rate);
    return;

out_irq:
    free_irq((int)ad_irq);
out_free:
    seg_put(ad_buf_seg);
    ad_buf_seg = NULL;
}

#endif /* CONFIG_CHAR_DEV_DSP_AD1848 */
