/*
 * arch/i86/286pmode/descriptor.h
 *
 * descriptor structure and access macros
 */

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

/* this is the 286 descriptor format */
struct descriptor {
    unsigned short limit;
    unsigned short baseaddr0;
    unsigned char baseaddr1;
    unsigned char flags;
    unsigned short reserved;
};

#endif
