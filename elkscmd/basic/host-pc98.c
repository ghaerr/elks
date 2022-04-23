/*
 * Architecture Specific routines for PC-98
 */
#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "basic.h"
#include "arch/io.h"

#define LIO_FAR_ADDR 0xF9900000
#define INT_FAR_ADDR 0x00000280 // int 0xA0

#define LIO_M_SIZE 5200

void (*intc5_h_p)(void);

unsigned char __far *lio_m;
unsigned int lio_m_seg;

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

void intc5_handler(void)
{
    printf("intC5\n");
    __asm__ volatile ("iret");
}

void host_lio98_init(void) {
    unsigned long __far *intvec;
    unsigned int __far *lioaddr;
    unsigned int i;

    intvec = (unsigned long __far *) INT_FAR_ADDR;

    lioaddr = (unsigned int __far *) (LIO_FAR_ADDR + 6);

    // Set interrupt vector 0xA0 - 0xAF
    for (i = 0; i < 16; i++) {
        *intvec = 0xF9900000 | (*lioaddr);
        intvec++;
        lioaddr += 2;
    }

    // Set interrupt handler for 0xC5
    intc5_h_p = intc5_handler;
    intvec = (unsigned long __far *) 0x00000314;
    *intvec = (unsigned long) ((unsigned long __far *) intc5_h_p);
    //printf("intc5_h_p %p intvec %lx\n", intc5_h_p, *intvec);

    // Allocate memory for LIO
    lio_m = (unsigned char __far *) malloc(LIO_M_SIZE);
    //printf("lio_m : %lx\n", (long) lio_m);

    lio_m_seg = (unsigned int) ((((unsigned long) lio_m) >> 16) + ((((unsigned long) lio_m) & 0xFFFF) >> 4) + 1);
    //printf("lio_m_seg : %x\n", lio_m_seg);

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

void host_lio98_plot(unsigned int x, unsigned int y) {

    y = 399 - y;

    lio_m[0] = (unsigned char) (x & 0xFF);
    lio_m[1] = (unsigned char) (x >> 8);
    lio_m[2] = (unsigned char) (y & 0xFF);
    lio_m[3] = (unsigned char) (y >> 8);
    lio_m[4] = (unsigned char) 7; // Pallet Number
    int_A6(lio_m_seg);
}
