/*
 * sbmix - read and set Sound Blaster mixer levels
 *
 * usage: sbmix [-p port] [-m level] [-v level] [-r reg] [-s reg=val]
 *
 * The /dev/dsp driver has no mixer ioctl: it sets master and voice to full
 * scale once at init and treats level as purely a property of the samples.
 * That is the right default for a card whose output is wired to headphones,
 * but on a board with its own amplifier every gain stage at maximum clips.
 * This reads the mixer back so the actual levels can be seen, and sets them so
 * a sane value can be found by ear before baking it into the driver.
 *
 * Port I/O from user space works because ELKS runs the 8086 in real mode with
 * no I/O privilege level, the same way beep(1) drives the PIT.  Reading the
 * mixer is harmless while the driver is idle; do not run it during playback.
 *
 * The level registers are two 4-bit fields on both the SB Pro and the SB16,
 * bits 7-4 for left and 3-0 for right, so one decoding covers both.  Linux's
 * sbpro_mix table is the reference: MIX_ENT(SOUND_MIXER_VOLUME, 0x22, 7, 4,
 * 0x22, 3, 4) is a bit offset of 7 and a width of 4.  The microphone level is
 * the exception at 3 bits, MIX_ENT(SOUND_MIXER_MIC, 0x0a, 2, 3, ...).
 *
 * A Sound Blaster 1.x or 2.0 (DSP 1.xx/2.xx) has no mixer at all.
 *
 * Register 0x0E is not a level: bit 1 selects stereo and bit 5 switches the
 * output filter off, and 0 in both is mono with the filter engaged.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "arch/io.h"

#define DEF_PORT        0x220

#define SB_RESET        0x06
#define SB_READ_DATA    0x0A
#define SB_WRITE_DATA   0x0C
#define SB_READ_STATUS  0x0E
#define SB_MIXER_ADDR   0x04
#define SB_MIXER_DATA   0x05

#define DSP_GET_VERSION 0xE1
#define DSP_READY       0xAA

#define MIX_RESET       0x00
#define MIX_VOICE       0x04
#define MIX_MIC         0x0A
#define MIX_FILTER      0x0E
#define MIX_MASTER      0x22
#define MIX_FM          0x26
#define MIX_CD          0x28
#define MIX_LINE        0x2E

static unsigned int base = DEF_PORT;
static int ver_major, ver_minor;

/* Spin rather than sleep: these waits are microseconds on real hardware. */
static int dsp_write(unsigned char v)
{
    int i;

    for (i = 0; i < 30000; i++) {
        if ((inb(base + SB_WRITE_DATA) & 0x80) == 0) {
            outb(v, base + SB_WRITE_DATA);
            return 0;
        }
    }
    return -1;
}

static int dsp_read(unsigned char *v)
{
    int i;

    for (i = 0; i < 30000; i++) {
        if ((inb(base + SB_READ_STATUS) & 0x80) != 0) {
            *v = inb(base + SB_READ_DATA);
            return 0;
        }
    }
    return -1;
}

static int dsp_reset(void)
{
    int i;
    unsigned char v;

    outb(1, base + SB_RESET);
    for (i = 0; i < 1000; i++)
        (void)inb(base + SB_READ_STATUS);
    outb(0, base + SB_RESET);
    for (i = 0; i < 30000; i++) {
        if ((inb(base + SB_READ_STATUS) & 0x80) != 0) {
            v = inb(base + SB_READ_DATA);
            if (v == DSP_READY)
                return 0;
        }
    }
    return -1;
}

/*
 * The index and data ports need a delay between them.  The OPTi 82C929
 * reference driver waits a millisecond either side of the data access
 * (Knipperts, UNITS/SBPRO.PAS); without it the write is lost and reads return
 * junk, which is what made this card look like it was ORing 0x11 into every
 * register.
 */
#define MIXER_DELAY 1200

static void mix_delay(void)
{
    int i;

    for (i = 0; i < MIXER_DELAY; i++)
        inb(0x61);
}

static unsigned char mix_read(unsigned char reg)
{
    unsigned char v;

    outb(reg, base + SB_MIXER_ADDR);
    mix_delay();
    v = inb(base + SB_MIXER_DATA);
    mix_delay();
    return v;
}

static void mix_write(unsigned char reg, unsigned char val)
{
    outb(reg, base + SB_MIXER_ADDR);
    mix_delay();
    outb(val, base + SB_MIXER_DATA);
    mix_delay();
}

/* Percentage of full scale, decoded for whichever layout this card uses.
 * A DSP 3.xx card packs two 3-bit fields at bits 7-5 and 3-1 - bits 4 and 0
 * do not exist in its register file and read back as 1 - so decoding it as
 * 4-bit reported 0xFF as "15/15" when the level is physically 7/7. */
static void show(const char *name, unsigned char reg)
{
    unsigned char v = mix_read(reg);
    int l, r, steps;

    if (ver_major == 3) {
        l = (v >> 5) & 0x07;
        r = (v >> 1) & 0x07;
        steps = 7;
    } else {
        l = (v >> 4) & 0x0F;
        r = v & 0x0F;
        steps = 15;
    }
    printf("  %-8s reg 0x%02x = 0x%02x   left %2d/%d (%3d%%)  right %2d/%d (%3d%%)\n",
        name, reg, v, l, steps, l * 100 / steps, r, steps, r * 100 / steps);
}

/* Build a register value from a 0-100 percentage in the card's own layout. */
static unsigned char level_to_reg(int pct)
{
    int n;

    /* clamp before the multiply: pct >= 2182 overflows 16-bit int in pct*15 */
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    if (ver_major == 3) {               /* 3-bit fields at 7-5 and 3-1 */
        n = (pct * 7 + 50) / 100;
        return (unsigned char)((n << 5) | (n << 1));
    }
    n = (pct * 15 + 50) / 100;          /* same scaling as Linux change_bits */
    return (unsigned char)((n << 4) | n);
}

/*
 * Mute every input that is not the PCM path.  A playback-only driver has no use
 * for the FM synthesiser, CD, line or microphone inputs, and whatever the card
 * powers up with is mixed into the same output as the PCM.  On a board whose FM
 * section is never initialised that is added noise for nothing.
 */
static void mute_inputs(void)
{
    mix_write(MIX_FM, 0x00);
    mix_write(MIX_CD, 0x00);
    mix_write(MIX_LINE, 0x00);
    mix_write(MIX_MIC, 0x00);
}

/*
 * AD1848 codec behind the OPTi's Sound Blaster personality.  Only reachable
 * when MC5 SPACCESS is set.  The access idiom is the vendor driver's
 * (Knipperts, AD1848.PAS ReadCODECReg): preserve the upper nibble of the index
 * register - it holds INIT, MCE and TRD - and read the data port three times,
 * which that driver marks "Errata for AD1848".
 */
#define WSS_BASE        0x530
#define CODEC_BASE      (WSS_BASE + 4)      /* index, data, status, PIO */

static const char *codec_rate(unsigned char v)
{
    static const char *xtal1[8] = { "8000", "16000", "27428", "32000",
                                    "n/a", "n/a", "48000", "9600" };
    static const char *xtal2[8] = { "5512", "11025", "18900", "22050",
                                    "37800", "44100", "33075", "6620" };
    unsigned char cfs = (unsigned char)((v >> 1) & 0x07);

    return (v & 0x01)? xtal2[cfs] : xtal1[cfs];
}

static int codec_ready(void)
{
    int i;

    for (i = 0; i < 30000; i++)
        if (inb(CODEC_BASE) != 0x80)        /* 0x80 = still initialising */
            return 0;
    return -1;
}

static unsigned char codec_read(unsigned char reg)
{
    unsigned char old, v;

    old = inb(CODEC_BASE);
    outb((unsigned char)((old & 0xF0) | (reg & 0x0F)), CODEC_BASE);
    mix_delay();
    v = inb(CODEC_BASE + 1);
    v = inb(CODEC_BASE + 1);
    v = inb(CODEC_BASE + 1);            /* errata: read three times */
    outb(old, CODEC_BASE);
    mix_delay();
    return v;
}

static void codec_dump(void)
{
    unsigned char i8, i9, i6, i7, st;

    if (codec_ready() < 0) {
        printf("codec: not answering at 0x%x", CODEC_BASE);
        printf(" (MC5 SPACCESS clear, or not a MAD16)\n");
        return;
    }
    i8 = codec_read(8);
    i9 = codec_read(9);
    i6 = codec_read(6);
    i7 = codec_read(7);
    st = inb(CODEC_BASE + 2);

    printf("codec at 0x%x (index reg = 0x%02x)\n", CODEC_BASE, inb(CODEC_BASE));
    printf("  I8  format 0x%02x  %s, %s, %s Hz\n", i8,
        (i8 & 0x10)? "STEREO" : "mono",
        (i8 & 0x20)? ((i8 & 0x40)? "8-bit A-law" : "8-bit u-law")
                   : ((i8 & 0x40)? "16-bit signed" : "8-bit unsigned"),
        codec_rate(i8));
    printf("  I9  iface  0x%02x  playback %s, %s, %s\n", i9,
        (i9 & 0x01)? "ENABLED" : "off",
        (i9 & 0x40)? "PIO" : "DMA",
        (i9 & 0x04)? "single-DMA" : "dual-DMA");
    printf("  I6  left DAC  0x%02x  %s, -%d.5 dB\n", i6,
        (i6 & 0x80)? "MUTED" : "on", (i6 & 0x3F) * 3 / 2);
    printf("  I7  right DAC 0x%02x  %s, -%d.5 dB\n", i7,
        (i7 & 0x80)? "MUTED" : "on", (i7 & 0x3F) * 3 / 2);
    printf("  status 0x%02x\n", st);
}

static void usage(void)
{
    fprintf(stderr, "usage: sbmix [-p port] [-m pct] [-v pct] [-r reg]"
        " [-s reg=val] [-z]\n");
    fprintf(stderr, "       -m master level 0-100, -v voice (PCM) level 0-100\n");
    fprintf(stderr, "       -r dump one register, -s write one raw register\n");
    fprintf(stderr, "       -z mute the fm, cd, line and mic inputs\n");
    fprintf(stderr, "       -k dump the AD1848 codec behind the SB personality\n");
    fprintf(stderr, "       note: every run resets the DSP, which on some cards\n");
    fprintf(stderr, "       restores mixer defaults - so pass all changes at once\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int c;
    long master = -1, voice = -1, one = -1;
    unsigned char v;
    char *set = NULL;
    int zap = 0, kdump = 0;

    while ((c = getopt(argc, argv, "p:m:v:r:s:zk")) != -1) {
        switch (c) {
        case 'p': base = (unsigned int)strtol(optarg, (char **)0, 0); break;
        case 'm': master = atol(optarg); break;
        case 'v': voice = atol(optarg); break;
        case 'r': one = strtol(optarg, (char **)0, 0); break;
        case 's': set = optarg; break;
        case 'z': zap = 1; break;
        case 'k': kdump = 1; break;
        default: usage();
        }
    }
    if (optind < argc)
        usage();

    if (dsp_reset() < 0) {
        fprintf(stderr, "sbmix: no card at 0x%x\n", base);
        return 1;
    }
    if (dsp_write(DSP_GET_VERSION) == 0) {
        unsigned char hi, lo;
        if (dsp_read(&hi) == 0 && dsp_read(&lo) == 0) {
            ver_major = hi;
            ver_minor = lo;
        }
    }
    printf("sb: dsp %d.%02d at 0x%x", ver_major, ver_minor, base);
    if (ver_major == 0 || ver_major >= 3)
        printf(", %s mixer, %s levels\n",
            (ver_major >= 4)? "SB16" : "SB Pro",
            (ver_major == 3)? "3-bit" : "4-bit");
    else {
        printf(", no mixer on this card\n");
        /* a requested change was refused, and a script should see that */
        return (set || zap || master >= 0 || voice >= 0)? 1 : 0;
    }

    if (set) {
        char *eq = strchr(set, '=');
        if (!eq)
            usage();
        *eq = '\0';
        mix_write((unsigned char)strtol(set, (char **)0, 0),
                  (unsigned char)strtol(eq + 1, (char **)0, 0));
    }
    if (zap)
        mute_inputs();
    if (master >= 0)
        mix_write(MIX_MASTER, level_to_reg((int)master));
    if (voice >= 0)
        mix_write(MIX_VOICE, level_to_reg((int)voice));

    if (one >= 0) {
        printf("  reg 0x%02x = 0x%02x\n", (unsigned)one,
            mix_read((unsigned char)one));
        return 0;
    }

    if (kdump)
        codec_dump();

    show("master", MIX_MASTER);
    show("voice", MIX_VOICE);
    show("fm", MIX_FM);
    show("cd", MIX_CD);
    show("line", MIX_LINE);
    printf("  %-8s reg 0x%02x = 0x%02x\n", "mic", MIX_MIC, mix_read(MIX_MIC));
    v = mix_read(MIX_FILTER);
    printf("  %-8s reg 0x%02x = 0x%02x   %s, output filter %s\n", "outmode",
        MIX_FILTER, v, (v & 0x02)? "STEREO" : "mono",
        (v & 0x20)? "off" : "on");
    return 0;
}
