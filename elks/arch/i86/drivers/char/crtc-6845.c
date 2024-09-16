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

struct hw_params crtc_params[N_DEVICETYPES] = {
    { 80, 25, 0x3B4, 0xB000, 0x1000, 2000, 1, 16,   /* MDA */
        {
            0x61, 0x50, 0x52, 0x0F,
            0x19, 0x06, 0x19, 0x19,
            0x02, 0x0D, 0x0B, 0x0C,
            0x00, 0x00, 0x00, 0x00,
        }
    },
    { 80, 25, 0x3D4, 0xB800, 0x4000, 2000, 3, 16,   /* CGA */
        {
            /* CO80 */
            0x71, 0x50, 0x5A, 0x0A,
            0x1F, 0x06, 0x19, 0x1C,
            0x02, 0x07, 0x06, 0x07,
            0x00, 0x00, 0x00, 0x00,
        }
    }
                                                    /* EGA (TODO) */
};

/* Check to see if this CRTC is present */
int INITPROC crtc_probe(unsigned short crtc_base)
{
    int i;
    unsigned char test = 0x55;
    unsigned char original, value;

    /* We'll try writing a value to cursor address LSB reg (0x0F), then reading it back */
    outb(0x0F, crtc_base + CRTC_INDX);
    original = inb(crtc_base + CRTC_DATA);
    if (original == test)
        test += 0x10;
    outb(0x0F, crtc_base + CRTC_INDX);
    outb_p(test, crtc_base + CRTC_DATA);
    outb(0x0F, crtc_base + CRTC_INDX);
    value = inb(crtc_base + CRTC_DATA);
    if (value != test)
        return 0;
    outb(0x0F, crtc_base + CRTC_INDX);
    outb(original, crtc_base + CRTC_DATA);
    return 1;
}

void INITPROC crtc_init(int dev)
{
    int i;
    struct hw_params *p = &crtc_params[dev];

    /* Set 80x25 mode, video off */
    outb(0x01, p->crtc_base + CRTC_MODE);
    /* Program CRTC regs */
    for (i = 0; i < p->n_init_bytes; ++i) {
        outb(i, p->crtc_base + CRTC_INDX);
        outb(p->init_bytes[i], p->crtc_base + CRTC_DATA);
    }

    /* Clear vram */
#if 0   //FIXME remove: return value never checked, useless code
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, p->vseg_base, 0x5555);
    for (i = 0; i < p->vseg_bytes; i += 2)
        if (peekw(i, p->vseg_base) != 0x5555) return 1;
#endif
    for (i = 0; i < p->vseg_bytes; i += 2)
        pokew(i, p->vseg_base, 0x07 << 8 | ' ');

    /* Enable video */
    outb(0x09, p->crtc_base + CRTC_MODE);
}
