/*
 * tss.h
 *
 * task state segment structure
 */

#ifndef TSS_H
#define TSS_H

/* this is the 286 TSS format */
struct tss {
    unsigned short backlink;
    unsigned short cpl0_sp;
    unsigned short cpl0_ss;
    unsigned short cpl1_sp;
    unsigned short cpl1_ss;
    unsigned short cpl2_sp;
    unsigned short cpl2_ss;
    unsigned short ip;
    unsigned short flags;
    unsigned short ax;
    unsigned short cx;
    unsigned short dx;
    unsigned short bx;
    unsigned short sp;
    unsigned short bp;
    unsigned short si;
    unsigned short di;
    unsigned short es;
    unsigned short cs;
    unsigned short ss;
    unsigned short ds;
    unsigned short ldt;
};

#endif
