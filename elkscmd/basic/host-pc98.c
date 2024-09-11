/*
 * Architecture Specific routines for PC-98
 */
#include <stdio.h>
#include <stdlib.h>
#include <arch/io.h>

#include "host.h"
#include "basic.h"

#define LIOSEG 0xF990 /* ROM Segment for LIO */
#define LIOINT 0xA0   /* Starting LIO interrupt number */

#define LIO_M_SIZE 5200

extern FILE *outfile;
extern void intc5_handler(void);

static unsigned char __far *lio_m;
static unsigned char lio_m_byte[LIO_M_SIZE];
static unsigned int lio_m_seg;

static int gmode = 0;

typedef struct {
    int x;
    int y;
    int fgc;
    int bgc;
    int r;
} xyc_t;

static xyc_t gxyc = {0, 0, 7, 0, 1};
static unsigned char cga[16] = {0x0, 0x1, 0x4, 0x5, 0x2, 0x3, 0x6, 0x7,
                                0x8, 0x9, 0xC, 0xD, 0xA, 0xB, 0xE, 0xF};

void int_A0(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "int $0xA0;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A1(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA1;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A2(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA2;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A3(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA3;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A5(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x0000,%%bx;"
                      "int $0xA5;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A6(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x01,%%ah;"
                      "mov $0x0000,%%bx;"
                      "int $0xA6;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A7(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x01,%%ah;"
                      "mov $0x0000,%%bx;"
                      "int $0xA7;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void int_A8(unsigned int l_seg)
{
    __asm__ volatile ("push %ds;"
                      "push %es;"
                      "push %bp;"
                      "push %si;"
                      "push %di;");
    __asm__ volatile ("mov %0,%%ds;"
                      "mov $0x01,%%ah;"
                      "mov $0x0000,%%bx;"
                      "int $0xA8;"
                      :
                      :"a" (l_seg)
                      :"memory", "cc");
    __asm__ volatile ("pop %di;"
                      "pop %si;"
                      "pop %bp;"
                      "pop %es;"
                      "pop %ds;");
}

void host_mode(int mode) {
    unsigned long __far *intvec;
    unsigned int __far *lioaddr;
    unsigned int i;

    gmode = mode;

    if (gmode) {

        intvec = (unsigned long __far *) _MK_FP(0, LIOINT<<2); /* interrupt vector for INT 0xA0 */
        lioaddr = (unsigned int __far *) _MK_FP(LIOSEG, 6);   /* Starting Rom Address for INT 0xA0 handler */

        // Set interrupt vector 0xA0 - 0xAF
        for (i = 0; i < 16; i++) {
            *intvec = (unsigned long) _MK_FP(LIOSEG, *lioaddr);
            intvec++;
            lioaddr += 2;
        }

        // Set interrupt handler for 0xC5
        intvec = _MK_FP(0, 0xC5 << 2);
        *intvec = (unsigned long) ((unsigned long __far *) intc5_handler);

        // Allocate memory for LIO
        lio_m = (unsigned char __far *) &lio_m_byte;

        lio_m_seg = (unsigned int) ((((unsigned long) lio_m) >> 16) + ((((unsigned long) lio_m) & 0xFFFF) >> 4) + 1);

        lio_m = (unsigned char __far *) (((unsigned long) lio_m_seg) << 16);

        int_A0(lio_m_seg); // Init

        lio_m[0] = 0x03; // Color 640x400
        lio_m[1] = 0x00;
        lio_m[2] = 0x00;
        lio_m[3] = 0x01;
        int_A1(lio_m_seg);

        lio_m[0] = 0x00;
        lio_m[1] = 0x00;
        lio_m[2] = 0x00;
        lio_m[3] = 0x00;
        lio_m[4] = 0x7F; // 639
        lio_m[5] = 0x02;
        lio_m[6] = 0x8F; // 399
        lio_m[7] = 0x01;
        lio_m[8] = 0xFF;
        lio_m[9] = 0xFF;
        int_A2(lio_m_seg);

        lio_m[0] = 0xFF;
        lio_m[1] = 0xFF;
        lio_m[2] = 0xFF;
        lio_m[3] = 0x02; // 16 Color mode
        int_A3(lio_m_seg);
    }
}

void host_cls() {
    fprintf(outfile, "\033[H\033[2J");

    if (gmode) {
        int_A5(lio_m_seg);
    }
}

void host_color(int fgc, int bgc) {

    if (gmode == 1) {
        gxyc.fgc = fgc;
        gxyc.bgc = bgc;
    }
    else if (gmode == 2) {
        gxyc.fgc = cga[fgc];
        gxyc.bgc = cga[bgc];
    }
}

void host_plot(int x, int y) {

    if (gmode) {
        y = 399 - y;

        lio_m[0] = (unsigned char) (x & 0xFF);
        lio_m[1] = (unsigned char) (x >> 8);
        lio_m[2] = (unsigned char) (y & 0xFF);
        lio_m[3] = (unsigned char) (y >> 8);
        lio_m[4] = (unsigned char) (gxyc.fgc); // Pallet Number
        int_A6(lio_m_seg);

        gxyc.x = x;
        gxyc.y = y;
    }
}

void host_draw(int x, int y) {

    if (gmode) {
        y = 399 - y;

        lio_m[0] = (unsigned char) (gxyc.x & 0xFF); // X start
        lio_m[1] = (unsigned char) (gxyc.x >> 8);
        lio_m[2] = (unsigned char) (gxyc.y & 0xFF); // Y start
        lio_m[3] = (unsigned char) (gxyc.y >> 8);
        lio_m[4] = (unsigned char) (x & 0xFF);      // X End
        lio_m[5] = (unsigned char) (x >> 8);
        lio_m[6] = (unsigned char) (y & 0xFF);      // Y End
        lio_m[7] = (unsigned char) (y >> 8);
        lio_m[8] = (unsigned char) (gxyc.fgc);      // Pallet Number
        lio_m[9] = (unsigned char) 0x00;
        lio_m[10] = (unsigned char) 0x00;
        lio_m[11] = (unsigned char) 0x00;
        lio_m[12] = (unsigned char) 0x00;
        lio_m[13] = (unsigned char) 0x00;
        lio_m[14] = (unsigned char) 0x00;
        lio_m[15] = (unsigned char) 0x00;
        lio_m[16] = (unsigned char) 0x00;
        lio_m[17] = (unsigned char) 0x00;
        int_A7(lio_m_seg);

        gxyc.x = x;
        gxyc.y = y;
    }
}

void host_circle(int x, int y, int r) {

    if (gmode) {
        y = 399 - y;

        lio_m[0] = (unsigned char) (x & 0xFF); // X Center
        lio_m[1] = (unsigned char) (x >> 8);
        lio_m[2] = (unsigned char) (y & 0xFF); // Y Center
        lio_m[3] = (unsigned char) (y >> 8);
        lio_m[4] = (unsigned char) (r & 0xFF); // X Radius
        lio_m[5] = (unsigned char) (r >> 8);
        lio_m[6] = (unsigned char) (r & 0xFF); // Y Radius
        lio_m[7] = (unsigned char) (r >> 8);
        lio_m[8] = (unsigned char) (gxyc.fgc); // Pallet Number
        lio_m[9] = (unsigned char) 0x00;
        lio_m[10] = (unsigned char) 0x00;
        lio_m[11] = (unsigned char) 0x00;
        lio_m[12] = (unsigned char) 0x00;
        lio_m[13] = (unsigned char) 0x00;
        lio_m[14] = (unsigned char) 0x00;
        lio_m[15] = (unsigned char) 0x00;
        lio_m[16] = (unsigned char) 0x00;
        lio_m[17] = (unsigned char) 0x00;
        lio_m[18] = (unsigned char) 0x00;
        lio_m[19] = (unsigned char) 0x00;
        lio_m[20] = (unsigned char) 0x00;
        lio_m[21] = (unsigned char) 0x00;
        lio_m[22] = (unsigned char) 0x00;
        int_A8(lio_m_seg);

        gxyc.r = r;
    }
}

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}

int host_analogRead(int pin) {
	return 0;
}

void host_pinMode(int pin,int mode) {
}

void host_outb(int port, int value) {
    outb(value, port);
}

void host_outw(int port, int value) {
    outw(value, port);
}

int host_inpb(int port) {
    return inb(port);
}

int host_inpw(int port) {
    return inw(port);
}
