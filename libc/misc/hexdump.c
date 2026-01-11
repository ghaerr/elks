/*
 * hexdump - display buffer in hex and ascii
 *
 * void hexdump(void *off, unsigned int seg, int count, int flags, const char *prefix)
 *  flags & 1:  combine duplicate lines and show '*' instead
 *  flags & 2:  display linear address rather than seg:offset
 */

#ifdef __KERNEL__
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define printk  printf
#endif

#if !defined(__ia16__) && !defined(__WATCOMC__)
#define __far
#endif

#define isprint(c) ((c) > ' ' && (c) <= '~')
static int lastnum[16] = {-1};
static long lastaddr = -1;

static void printline(unsigned int offset, unsigned int seg, int *num, char *chr,
    int count, int flags, const char *prefix)
{
    long address = ((unsigned long)seg << 4) + offset;
    int j;

    if (lastaddr >= 0) {
        for (j = 0; j < count; j++)
            if (num[j] != lastnum[j])
                break;
        if (j == 16 && (flags & 1)) {
            if (lastaddr + 16 == address)
                printk("*\n");
            return;
        }
    }

    lastaddr = address;
    if (flags & 2)
        printk("%s%06lx:", prefix, address);
    else printk("%s%04x:%04x", prefix, seg, offset);
    for (j = 0; j < count; j++) {
        if (j == 8)
            printk(" ");
        if (num[j] >= 0)
            printk(" %02x", num[j]);
        else
            printk("   ");
        lastnum[j] = num[j];
        num[j] = -1;
    }

    for (j=count; j < 16; j++) {
        if (j == 8)
            printk(" ");
        printk("   ");
    }

    printk("  ");
    for (j = 0; j < count; j++)
        printk("%c", chr[j]);
    printk("\n");
}

void hexdump(void *off, unsigned int seg, int count, int flags, const char *prefix)
{
    unsigned char __far *addr;
    unsigned int offset = (unsigned)off;
    static char buf[20];
    static int num[16];

    if (!prefix) prefix = "";
    addr = (unsigned char __far *)(((unsigned long)seg << 16) | (unsigned int)off);
    lastnum[0] = -1;
    lastaddr = -1;
    for (; count > 0; count -= 16, offset += 16) {
        int j, ch;

        memset(buf, 0, 16);
        for (j = 0; j < 16; j++)
            num[j] = -1;
        for (j = 0; j < 16; j++) {
            ch = *addr++;
            num[j] = ch;
            if (isprint(ch) && ch < 0x80)
                buf[j] = ch;
            else
                buf[j] = '.';
        }
        printline(offset, seg, num, buf, count > 16? 16: count, flags, prefix);
    }
}
