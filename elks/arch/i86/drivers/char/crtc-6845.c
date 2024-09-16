#include <arch/io.h>
#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/memory.h>
#include "crtc-6845.h"

/*
 * https://stanislavs.org/helppc/6845.html
 * https://www.seasip.info/VintagePC/index.html
 *
 *     I/O          VidBase  VidLen       Notes
 * MDA: 0x3B0-0x3BB 0xB0000  4k  (0x1000)
 * CGA: 0x3D0-0x3DC 0xB8000  16k (0x4000) note: Address not fully decoded, fb is also at 0xBC000
 * EGA: 0x3C0-0x3CF ???????  ?????
 * VGA: ?????
 */

#define CRTC_INDX 0x0
#define CRTC_DATA 0x1
#define CRTC_MODE 0x4
#define CRTC_CSEL 0x5
#define CRTC_STAT 0x6

/* Check to see if this CRTC is present */
int INITPROC crtc_probe(unsigned short crtc_base)
{
    int i;
    unsigned char test = 0x55;
    /* We'll try writing a value to cursor address LSB reg (0x0F), then reading it back */
    outb(0x0F, crtc_base + CRTC_INDX);
    unsigned char original = inb(crtc_base + CRTC_DATA);
    if (original == test) test += 0x10;
    outb(0x0F, crtc_base + CRTC_INDX);
    outb(test, crtc_base + CRTC_DATA);
    /* Now wait a bit */
    for (i = 0; i < 100; ++i) {} // TODO: Verify this doesn't get optimized out
    outb(0x0F, crtc_base + CRTC_INDX);
    unsigned char value = inb(crtc_base + CRTC_DATA);
    if (value != test) return 0;
    outb(0x0F, crtc_base + CRTC_INDX);
    outb(original, crtc_base + CRTC_DATA);
    return 1;
}

int INITPROC crtc_init(unsigned int t)
{
    int i;
    struct hw_params *p = &crtc_params[t];
    /* Set 80x25 mode, video off */
    outb(0x01, p->crtc_base + CRTC_MODE);
    /* Program CRTC regs */
    for (i = 0; i < p->n_init_bytes; ++i) {
        outb(i, p->crtc_base + CRTC_INDX);
        outb(p->init_bytes[i], p->crtc_base + CRTC_DATA);
    }

    /* Check & clear vram */
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, p->vseg_base, 0x5555);
    for (i = 0; i < p->vseg_bytes; i += 2)
        if (peekw(i, p->vseg_base) != 0x5555) return 1;
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, p->vseg_base, 0x7 << 8 | ' ');

    /* Enable video */
    outb(0x09, p->crtc_base + CRTC_MODE);
    return 0;
}

