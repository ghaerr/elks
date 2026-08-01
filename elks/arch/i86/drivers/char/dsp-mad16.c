/*
 * OPTi 82C929 (MAD16 Pro) early initialisation for the /dev/dsp driver
 *
 * A MAD16 board answers Sound Blaster DSP commands at the usual ports, but
 * only once the OPTi control registers have been programmed to route the SB
 * personality to a port, IRQ and DMA channel.  A DOS driver normally does
 * that at boot; when nothing has, the card looks present but never transfers.
 * This file does the same bring-up as the Linux mad16.c C929 path, cut down to
 * what an SB-only playback driver needs, and is only reached when /bootopts
 * asks for it with mad16=on.
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

#if defined(CONFIG_CHAR_DEV_DSP) && defined(CONFIG_SB_MAD16)

#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/init.h>
#include <linuxmt/string.h>
#include <arch/io.h>
#include <arch/irq.h>

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
#define MC3_IRQ_MASK    0xC0
#define MC3_DMA_MASK    0x30
#define MC3_SB_240      0x04
#define MC3_GP_TIMER    0x02        /* write GPMODE; read overlaps REV bit 1 */
#define MC3_SB_OFF      0xF0        /* SB disabled while WSS is being set up */
#define MC4_ADPCM       0x80
#define MC4_TIMEOUT     0x20
#define MC4_SBVER_3     0x02
/*
 * MC5 is the diagnostic register, and two of these bits were previously named
 * as though they were reserved.  They are not: bit 7 is AUTOVOL and bit 2 is the
 * Sound Blaster mixer enable.  The vendor driver makes the polarity explicit -
 * "if (getbit(mc5data,7) = 0) then cfg.autovol := 1; {low means active here}" -
 * so setting bit 7 DISABLES automatic volume.
 *
 * The datasheet gives a normal setting per codec: 0x2F for a CS4231/4248 and
 * 0x25 for an AD1848/1846, the difference being CFIX, the fix for Crystal part
 * synchronisation delays, which must be off for an AD1848.  SPACCESS stays clear
 * so the codec is not reachable while the chip is in Sound Blaster mode, which
 * is what the normal setting specifies.
 */
#define MC5_AUTOVOL_OFF 0x80        /* 1 = automatic volume disabled */
#define MC5_MUST0_6     0x40
#define MC5_SHPASS      0x20        /* protect the codec shadow registers */
#define MC5_SPACCESS    0x10        /* codec reachable during SB mode */
#define MC5_CFIFO       0x08
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
 * SPACCESS is deliberately NOT set.  It was tried, to make the codec readable at
 * WSS_BASE+4 during Sound Blaster mode so the format the emulation programs
 * could be inspected; the card then stopped answering the DSP at 0x220
 * altogether.  On this part the codec window and the SB personality are
 * mutually exclusive, so the datasheet's normal setting is a requirement rather
 * than just a default.  0x25 is that setting for an AD1848.
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
    static const unsigned char init_values[32] = {
        0xa8, 0xa8, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
        0x00, 0x0c, 0x02, 0x00, 0x8a, 0x01, 0x00, 0x00,
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



#endif /* CONFIG_CHAR_DEV_DSP && CONFIG_SB_MAD16 */
