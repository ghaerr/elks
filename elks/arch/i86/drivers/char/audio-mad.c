/*
 * OPTi 82C929 (MAD16 Pro) early initialisation for the /dev/dsp driver
 *
 * A MAD16 board answers Sound Blaster DSP commands at the usual ports, but
 * only once the OPTi control registers have been programmed to route the SB
 * personality to a port, IRQ and DMA channel.  A DOS driver normally does
 * that at boot; when nothing has, the card looks present but never transfers.
 * This file does the same bring-up as the Linux mad16.c C929 path, cut down to
 * what an SB-only playback driver needs, and is only reached when /bootopts
 * asks for it with mad16=irq,port,dma.
 *
 * The control registers are password gated: 0xE3 must be written to 0xF8F
 * immediately before each access, so every read and write here runs with
 * interrupts off.
 *
 * The attached WSS codec is initialised before the SB profile is written.
 * Without that step some boards accept DSP commands but never drain enough of
 * the internal FIFO to keep DRQ asserted, which looks like a dead DMA channel.
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/init.h>
#include <linuxmt/string.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/audio-sb.h>

/* Set to 1 for codec readback reporting at boot. */
#ifndef MAD16_DEBUG
#define MAD16_DEBUG 0
#endif

/*
 * All port access goes through the inb_p/outb_p bus-recovery forms except
 * where noted below: slow ISA parts drop back-to-back accesses, and the OPTi
 * password gate and the codec index/data pairs are both exposed to that.
 */

/*
 * Raw, unpadded port access for the password gate only.  The OPTi opens its
 * control registers for exactly one access after the password byte, and outb_p
 * emits its recovery cycle AFTER the write - which would put a write to port
 * 0x80 between the password and the register access and consume the gate.
 * Everything else in this file keeps the recovery cycle.
 */
#define MC_RAW_OUTB(value, port)                            \
    asm volatile ("outb %%al,%%dx"                          \
                  : : "Ral" ((unsigned char)(value)), "d" (port))
#define MC_RAW_INB(port) __extension__ ({                   \
    unsigned char _v;                                       \
    asm volatile ("inb %%dx,%%al" : "=Ral" (_v) : "d" (port)); \
    _v; })

/* MC1..MC6 control registers, password port, and the WSS window */
#define MC_PWD_PORT     0xF8F
#define MC_PASSWORD     0xE3
#define MC1             0xF8D
#define MC2             0xF8E
#define MC3             0xF8F
#define MC4             0xF90
#define MC5             0xF91
#define MC6             0xF92
#define WSS_BASE        0x530
#define CODEC_BASE      (WSS_BASE + 4)

#define MC1_DEFAULT     0x00
#define MC1_MASK        (0x40 | 0x01)
#define MC1_WSS_MODE    0x80        /* WSS mode, WSBase 0x530 */
#define MC2_DEFAULT     0x03
#define MC2_OPL4        0x20
#define MC2_CDSEL_OFF   0x03
/*
 * MC3 (0xF8F behind the password) is the register the whole Sound Blaster
 * personality hangs on: it routes the SB engine's IRQ and DMA, places its
 * base port, and picks how sample rates map onto the codec's two crystals.
 * Layout, from the 82C929 datasheet (Rev 1.0, pages 8-9):
 *
 *   bits 7:6  DAIRQ   SB interrupt     00=IRQ7  01=IRQ10  10=IRQ5  11=none
 *   bits 5:4  DADRQ   SB DMA channel   00=DRQ1  01=DRQ0   10=DRQ3  11=none
 *   bit  3    FMAP    rate mapping     0=Normal (both codec crystals)
 *                                      1=Single (16.9 MHz crystal only)
 *   bit  2    DABASE  SB base port     0=0x220  1=0x240
 *   bits 1:0            on READ: chip revision ID ("10" for this part)
 *   bit  1    GPMODE  on WRITE: game port timer  0=external 1=internal
 *
 * Datasheet normal setting is 0x02: IRQ 7, DRQ 1, base 0x220, Normal
 * mapping, internal game timer - and 0x02 is exactly what this driver
 * writes for the standard route, so every field below is a deliberate
 * choice, not a leftover.
 *
 * Two traps for the unwary:
 * - Bits 1:0 are not read-back-able state: reads return the chip revision,
 *   so an MC3 readback never confirms GPMODE and a shadow copy must be
 *   trusted instead.
 * - FMAP must stay 0.  In Single mode every SB rate is quantised to what
 *   the 16.9344 MHz crystal can produce, and the common telephone-quality
 *   rates cannot be: 8000 and 16000 Hz exist only on the 24.576 MHz
 *   crystal.  Normal mode is what makes "play -r 8000" land on
 *   exactly 8000 Hz.
 *
 * These four IRQs and three DRQs are the entire route space of a
 * jumperless card; anything else has no MC3 encoding.  On a machine whose
 * hard disk controller owns IRQ 5 and DRQ 3, that leaves exactly one
 * usable route: IRQ 7 with DRQ 1.
 */
#define MC3_SB_240      0x04
#define MC3_GP_TIMER    0x02        /* write GPMODE; read overlaps REV bit 1 */
#define MC3_SB_OFF      0xF0        /* SB disabled while WSS is being set up */
#define MC4_ADPCM       0x80
#define MC4_TIMEOUT     0x20
#define MC4_SBVER_3     0x02
/*
 * MC5 is the diagnostic register.  Bit 7 high DISABLES automatic volume.  The
 * datasheet's normal setting is 0x2F for a CS4231/4248 and 0x25 for an
 * AD1848/1846, the difference being CFIX, which must be off for an AD1848.
 */
#define MC5_SHPASS      0x20        /* protect the codec shadow registers */
#define MC5_SPACCESS    0x10        /* codec reachable during SB mode */
#define MC5_SBMIX       0x04        /* Sound Blaster mixer enable */
#define MC5_CFIX        0x02        /* 1 = CS4231/4248, 0 = AD1848/1846 */
#define MC5_CDFTOEN     0x01
#define MC6_MPU_ENABLE  0x80
#define MC6_MPU_IRQ     0x18
#define MC6_MPU_OFF     0x07
#define MC6_WAVE        0x02
#define MC6_ATTN        0x01

#define MC4_DEFAULT     (MC4_ADPCM | MC4_TIMEOUT | MC4_SBVER_3)
/*
 * SPACCESS must stay clear: the codec window and the SB personality are
 * mutually exclusive on this part, and setting it stops the DSP answering.
 * Bit 7 stays clear too, per the datasheet's printed normal setting: the
 * vendor sources contradict themselves about which polarity of the
 * "automatic volume" bit is off, and line-out capture measured LESS
 * output for LOUDER input with rising harmonics - gain pumping - while
 * the bit was set.  Datasheet-normal 0x25 measures monotonic.
 */
#define MC5_DEFAULT     (MC5_SHPASS | MC5_SBMIX | MC5_CDFTOEN)   /* 0x25 */
#define MC6_DEFAULT     (MC6_WAVE | MC6_ATTN)

#define CODEC_MCE       0x40        /* mode change enable in the index register */

/*
 * The SB profile is kept so it can be written again when /dev/dsp is opened:
 * anything else that has touched the chip in the meantime, including the WSS
 * side of the same board, may have moved the SB personality.
 */
static unsigned char mad16_profile_valid;
static unsigned char mad16_mc[6];

static void FARPROC mc_write(unsigned int port, unsigned char val)
{
    unsigned int flags;

    save_flags(flags);
    clr_irq();
    MC_RAW_OUTB(MC_PASSWORD, MC_PWD_PORT);
    MC_RAW_OUTB(val, port);
    restore_flags(flags);
}

static unsigned char INITPROC mc_read(unsigned int port)
{
    unsigned int flags;
    unsigned char val;

    save_flags(flags);
    clr_irq();
    MC_RAW_OUTB(MC_PASSWORD, MC_PWD_PORT);
    val = MC_RAW_INB(port);
    restore_flags(flags);
    return val;
}

static void FARPROC mc_write_profile(unsigned char *mc)
{
    mc_write(MC1, mc[0]);
    mc_write(MC2, mc[1]);
    mc_write(MC3, mc[2]);
    mc_write(MC4, mc[3]);
    mc_write(MC5, mc[4]);
    mc_write(MC6, mc[5]);
}

void FARPROC mad16_restore_profile(void)
{
    if (mad16_profile_valid)
        mc_write_profile(mad16_mc);
}

/*
 * Hold the codec in 8-bit unsigned linear PCM.
 *
 * The SB engine rewrites the codec's data format register whenever it maps
 * a sample rate, and it selects mu-law - which folds a linear waveform at
 * the 0x80 sign boundary, so a 440Hz tone comes out at 880Hz.  This is
 * called after every rate command to put the format back.
 *
 * Only the two companding bits are touched.  Widening the mask to the
 * whole format field costs playback outright: the stereo bit next to it
 * reads back set, so the condition below fires on every block, and
 * asserting MCE on a codec that is already streaming stalls the transfer
 * in progress - the block never completes and the writer blocks in the
 * driver for good.  The write is therefore made only when the readback
 * shows a companded format, which after the first repair of a stream is
 * almost never.
 */
#define CODEC_FMT_MASK  0x60    /* the companding format bits of I8 */

void FARPROC mad16_codec_fix_fmt(void)
{
    unsigned char v;

    if (!mad16_profile_valid)
        return;
    mc_write(MC5, (unsigned char)(mad16_mc[4] | MC5_SPACCESS));
    outb_p(8, CODEC_BASE);
    v = inb_p(CODEC_BASE + 1);
    if (v & CODEC_FMT_MASK) {
        /* MCE on the index access: the codec ignores format writes
         * without it.  ACAL is off in the codec setup, so leaving MCE
         * again does not start a recalibration that would mute us.
         */
        outb_p(0x48, CODEC_BASE);
        outb_p((unsigned char)(v & ~CODEC_FMT_MASK), CODEC_BASE + 1);
        outb_p(8, CODEC_BASE);
    }
    mc_write(MC5, mad16_mc[4]);
}

/*
 * Present if MC1 reads differently through the password gate than without it,
 * and if a toggled bit reads back through the gate.
 */
static int INITPROC mad16_detect(void)
{
    unsigned char mc1, ungated, toggled;

    mc1 = mc_read(MC1);
    if (mc1 == 0xFF)
        return 0;
    ungated = inb_p(MC1);
    if (ungated == mc1)
        return 0;

    mc_write(MC1, (unsigned char)(mc1 ^ 0x80));
    toggled = mc_read(MC1);
    mc_write(MC1, mc1);
    return toggled == (unsigned char)(mc1 ^ 0x80);
}

/*
 * The chip can only put the Sound Blaster personality on the handful of
 * resources MC3 has room to encode, so these are the only routes worth
 * attempting.  A card that has no jumpers is limited to exactly this set.
 */
static int INITPROC mad16_valid_route(unsigned int port, int irq, int dma)
{
    if (port != 0x220 && port != 0x240)
        return 0;
    if (irq != 5 && irq != 7)
        return 0;
    if (dma != 1 && dma != 3)
        return 0;
    return 1;
}

/*
 * MC3 holds the SB route in three fields:
 *
 *      bit    2    SB port, clear = 0x220, set = 0x240
 *      bits 7:6    IRQ, 00 = 7, 10 = 5, 11 = disabled, 01 invalid
 *      bits 5:4    DMA, 00 = 1, 10 = 3, 11 = disabled, 01 invalid
 *
 * mad16_valid_route has already rejected anything the fields cannot express,
 * so the invalid 01 pattern cannot be produced here.
 */
static unsigned char INITPROC mad16_mc3_for_sb(unsigned int port, int irq, int dma)
{
    unsigned char mc3 = MC3_GP_TIMER;

    if (irq == 5)
        mc3 |= 0x80;
    if (dma == 3)
        mc3 |= 0x20;
    if (port == 0x240)
        mc3 |= MC3_SB_240;
    return mc3;
}

static int INITPROC codec_wait(unsigned int base)
{
    unsigned int timeout = 60000;

    while (timeout-- != 0) {
        if ((inb_p(base) & 0x80) == 0)
            return 0;
        inb_p(0x61);                  /* ISA bus recovery delay */
    }
    return -EIO;
}

static unsigned char INITPROC codec_read(unsigned int base, unsigned char reg,
                                         unsigned char mce)
{
    unsigned int flags;
    unsigned char value;

    (void)codec_wait(base);
    save_flags(flags);
    clr_irq();
    outb_p((unsigned char)((reg & 0x1F) | mce), base);
    value = inb_p(base + 1);
    restore_flags(flags);
    return value;
}

static void INITPROC codec_write(unsigned int base, unsigned char reg,
                                 unsigned char value, unsigned char mce)
{
    unsigned int flags;

    (void)codec_wait(base);
    save_flags(flags);
    clr_irq();
    outb_p((unsigned char)((reg & 0x1F) | mce), base);
    outb_p(value, base + 1);
    restore_flags(flags);
}

/*
 * Leaving mode change enable takes a while to settle.  Two bounded passes keep
 * the original long wait without needing a 32-bit loop counter.
 */
static void INITPROC codec_leave_mce(unsigned int base)
{
    unsigned int timeout;
    unsigned char pass;

    (void)codec_wait(base);
    outb_p(0, base);
    outb_p(0, base);

    for (pass = 0; pass < 2; pass++) {
        timeout = 40000;
        while (timeout-- != 0 && (codec_read(base, 11, 0) & 0x20) != 0)
            inb_p(0x61);
    }
}

/*
 * Bring up the attached AD1848 class codec.  Returns 0 if no codec answered,
 * which is not fatal: the SB profile is still written.
 */
static int INITPROC codec_init(unsigned int base, unsigned char *mc5_extra)
{
    /*
     * Linux ad1848.c uses this table too, but is only safe with it because
     * ad1848_mixer_reset() immediately reprograms every gain and mute.  This
     * driver has no mixer path to the codec, and the SB profile written
     * afterwards locks the codec shadows, so whatever is set here is the
     * card's analog state forever.  So: ADC source line at 0 dB with the
     * mic boost off (I0/I1), every aux input muted (I2-I5, bit 7 is the
     * mute), and the ADC-to-DAC digital mix off (I13) - the vendor's SB
     * setup writes no codec registers at all and relies on these same
     * quiet defaults.  I0/I1 at 0xa8 plus I13 at 0x01 put the open mic
     * input, amplified about 32 dB, straight into the DAC: heard as loud
     * static under all playback, scaling with bus and disk activity.
     */
    static const unsigned char init_values[32] = {
        0x00, 0x00, 0x88, 0x88, 0x88, 0x88, 0x00, 0x00,
        0x00, 0x04, 0x02, 0x00, 0x8a, 0x00, 0x00, 0x00,
        0x80, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    unsigned char tmp, cs_compat = 0;
    unsigned int i;

    *mc5_extra = 0;
    if (codec_wait(base) < 0 || (inb_p(base) & 0x80) != 0)
        return 0;

    /* Writable-register check, as Linux ad1848_detect does */
    codec_write(base, 0, 0xaa, CODEC_MCE);
    codec_write(base, 1, 0x45, CODEC_MCE);
    if (codec_read(base, 0, CODEC_MCE) != 0xaa ||
        codec_read(base, 1, CODEC_MCE) != 0x45)
        return 0;
    codec_write(base, 0, 0x45, CODEC_MCE);
    codec_write(base, 1, 0xaa, CODEC_MCE);
    if (codec_read(base, 0, CODEC_MCE) != 0x45 ||
        codec_read(base, 1, CODEC_MCE) != 0xaa)
        return 0;

    codec_write(base, 12, 0x40, CODEC_MCE);
    tmp = codec_read(base, 12, CODEC_MCE);
    if (tmp & 0x80)
        cs_compat = 1;

    /*
     * Program every register to an explicit value.  The codec silently
     * ignores writes while its INIT or calibration state is active, so a
     * write is not proof the register took; MAD16_DEBUG adds a readback pass
     * that re-writes any mismatch and reports how many stayed stuck.
     */
    for (i = 0; i < 16; i++)
        codec_write(base, (unsigned char)i, init_values[i], CODEC_MCE);
    codec_write(base, 9, (unsigned char)(codec_read(base, 9, CODEC_MCE) | 0x04),
        CODEC_MCE);
    if ((tmp & 0xC0) == 0xC0) {
        codec_write(base, 12,
            (unsigned char)(codec_read(base, 12, CODEC_MCE) | 0x40), CODEC_MCE);
        for (i = 16; i < 32; i++)
            codec_write(base, (unsigned char)i, init_values[i], CODEC_MCE);
    }
#if MAD16_DEBUG
    {
        unsigned int bad = 0;
        unsigned char got;

        for (i = 0; i < 16; i++) {
            unsigned char want = init_values[i];
            unsigned char mask = 0xFF;

            if (i == 12)
                continue;               /* ID bits, not storage */
            if (i == 9) {
                want |= 0x04;
                mask = (unsigned char)~0x20;    /* ACI is read-only */
            }
            got = codec_read(base, (unsigned char)i, CODEC_MCE);
            if ((got & mask) != (want & mask)) {
                codec_write(base, (unsigned char)i, want, CODEC_MCE);
                got = codec_read(base, (unsigned char)i, CODEC_MCE);
                if ((got & mask) != (want & mask))
                    bad++;
            }
        }
        printk("mad16: codec i0=%x i2=%x i6=%x i13=%x stuck=%u\n",
            codec_read(base, 0, CODEC_MCE), codec_read(base, 2, CODEC_MCE),
            codec_read(base, 6, CODEC_MCE), codec_read(base, 13, CODEC_MCE),
            bad);
    }
#endif
    outb_p(0, base + 2);
    codec_leave_mce(base);

    if (cs_compat)
        *mc5_extra = MC5_CFIX;
    return 1;
}

int INITPROC mad16_early_init(unsigned int port, int irq, int dma)
{
    unsigned char mc[6];
    unsigned char mc5_extra = 0;

    mad16_profile_valid = 0;
    if (!mad16_valid_route(port, irq, dma))
        return -EINVAL;
    if (!mad16_detect())
        return -ENODEV;

    mc[0] = MC1_DEFAULT & MC1_MASK;
    mc[1] = (MC2_DEFAULT & MC2_OPL4) | MC2_CDSEL_OFF;
    mc[2] = mad16_mc3_for_sb(port, irq, dma);
    mc[3] = MC4_DEFAULT;
    mc[4] = MC5_DEFAULT;        /* datasheet normal setting for an AD1848 */
    mc[5] = MC6_DEFAULT;

    if ((mc[5] & MC6_MPU_ENABLE) == 0)
        mc[5] &= MC6_MPU_OFF;
    else if ((mc[5] & MC6_MPU_IRQ) == 0x00 || (mc[5] & MC6_MPU_IRQ) == 0x08)
        mc[5] = (unsigned char)((mc[5] & ~MC6_MPU_IRQ) | 0x18);

    /* WSS side first, with SB switched off while the codec is set up */
    mc_write(MC1, MC1_WSS_MODE);
    mc_write(MC2, mc[1]);
    mc_write(MC3, MC3_SB_OFF);
    mc_write(MC4, mc[3]);
    mc_write(MC5, mc[4]);
    mc_write(MC6, mc[5]);
    outb_p((dma == 3)? 0x03: 0x02, WSS_BASE);
    (void)codec_init(CODEC_BASE, &mc5_extra);

    mc[4] |= mc5_extra;
    mc_write_profile(mc);
    memcpy(mad16_mc, mc, sizeof(mad16_mc));
    mad16_profile_valid = 1;
    return 0;
}
