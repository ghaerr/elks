/* $Id$
 * linux/include/asm/dma.h: Defines for using and allocating dma channels.
 * Written by Hennus Bergman, 1992.
 * High DMA channel support & info by Hannu Savolainen
 * and John Boyd, Nov. 1992.
 */

#ifndef __ARCH_8086_DMA_H
#define __ARCH_8086_DMA_H

#include <arch/io.h>		/* need byte IO */

#ifdef HAVE_REALLY_SLOW_DMA_CONTROLLER
#define dma_outb	outb_p
#else
#define dma_outb	outb
#endif

#define dma_inb		inb

/*
 * NOTES about DMA transfers:
 *
 *  controller 1: channels 0-3, byte operations, ports 00-1F
 *  controller 2: channels 4-7, word operations, ports C0-DF
 *
 *  - ALL registers are 8 bits only, regardless of transfer size
 *  - channel 4 is not used - cascades 1 into 2.
 *  - channels 0-3 are byte - addresses/counts are for physical bytes
 *  - channels 5-7 are word - addresses/counts are for physical words
 *  - transfers must not cross physical 64K (0-3) or 128K (5-7) boundaries
 *  - transfer count loaded to registers is 1 less than actual count
 *  - controller 2 offsets are all even (2x offsets for controller 1)
 *  - page registers for 5-7 don't use data bit 0, represent 128K pages
 *  - page registers for 0-3 use bit 0, represent 64K pages
 *
 * DMA transfers are limited to the lower 16MB of _physical_ memory.  
 * Note that addresses loaded into registers must be _physical_ addresses,
 * not logical addresses (which may differ if paging is active).
 *
 *  Address mapping for channels 0-3:
 *
 *   A23 ... A16 A15 ... A8  A7 ... A0    (Physical addresses)
 *    |  ...  |   |  ... |   |  ... |
 *    |  ...  |   |  ... |   |  ... |
 *    |  ...  |   |  ... |   |  ... |
 *   P7  ...  P0  A7 ... A0  A7 ... A0   
 * |    Page    | Addr MSB | Addr LSB |   (DMA registers)
 *
 *  Address mapping for channels 5-7:
 *
 *   A23 ... A17 A16 A15 ... A9 A8 A7 ... A1 A0    (Physical addresses)
 *    |  ...  |   \   \   ... \  \  \  ... \  \
 *    |  ...  |    \   \   ... \  \  \  ... \  (not used)
 *    |  ...  |     \   \   ... \  \  \  ... \
 *   P7  ...  P1 (0) A7 A6  ... A0 A7 A6 ... A0   
 * |      Page      |  Addr MSB   |  Addr LSB  |   (DMA registers)
 *
 * Again, channels 5-7 transfer _physical_ words (16 bits), so addresses
 * and counts _must_ be word-aligned (the lowest address bit is _ignored_ at
 * the hardware level, so odd-byte transfers aren't possible).
 *
 * Transfer count (_not # bytes_) is limited to 64K, represented as actual
 * count - 1 : 64K => 0xFFFF, 1 => 0x0000.  Thus, count is always 1 or more,
 * and up to 128K bytes may be transferred on channels 5-7 in one operation. 
 *
 */

#define MAX_DMA_CHANNELS	8

/* The maximum address that we can perform a DMA transfer to on this platform */
#define MAX_DMA_ADDRESS      0x1000000

/* 8237 DMA controllers */
#define IO_DMA1_BASE	0x00	/* 8 bit slave DMA, channels 0..3 */
#define IO_DMA2_BASE	0xC0	/* 16 bit master DMA, ch 4(=slave input)..7 */

/* DMA controller registers */
#define DMA1_CMD_REG		0x08	/* command register (w) */
#define DMA1_STAT_REG		0x08	/* status register (r) */
#define DMA1_REQ_REG            0x09	/* request register (w) */
#define DMA1_MASK_REG		0x0A	/* single-channel mask (w) */
#define DMA1_MODE_REG		0x0B	/* mode register (w) */
#define DMA1_CLEAR_FF_REG	0x0C	/* clear pointer flip-flop (w) */
#define DMA1_TEMP_REG           0x0D	/* Temporary Register (r) */
#define DMA1_RESET_REG		0x0D	/* Master Clear (w) */
#define DMA1_CLR_MASK_REG       0x0E	/* Clear Mask */
#define DMA1_MASK_ALL_REG       0x0F	/* all-channels mask (w) */

#define DMA2_CMD_REG		0xD0	/* command register (w) */
#define DMA2_STAT_REG		0xD0	/* status register (r) */
#define DMA2_REQ_REG            0xD2	/* request register (w) */
#define DMA2_MASK_REG		0xD4	/* single-channel mask (w) */
#define DMA2_MODE_REG		0xD6	/* mode register (w) */
#define DMA2_CLEAR_FF_REG	0xD8	/* clear pointer flip-flop (w) */
#define DMA2_TEMP_REG           0xDA	/* Temporary Register (r) */
#define DMA2_RESET_REG		0xDA	/* Master Clear (w) */
#define DMA2_CLR_MASK_REG       0xDC	/* Clear Mask */
#define DMA2_MASK_ALL_REG       0xDE	/* all-channels mask (w) */

#define DMA_ADDR_0              0x00	/* DMA address registers */
#define DMA_ADDR_1              0x02
#define DMA_ADDR_2              0x04
#define DMA_ADDR_3              0x06
#define DMA_ADDR_4              0xC0
#define DMA_ADDR_5              0xC4
#define DMA_ADDR_6              0xC8
#define DMA_ADDR_7              0xCC

#define DMA_CNT_0               0x01	/* DMA count registers */
#define DMA_CNT_1               0x03
#define DMA_CNT_2               0x05
#define DMA_CNT_3               0x07
#define DMA_CNT_4               0xC2
#define DMA_CNT_5               0xC6
#define DMA_CNT_6               0xCA
#define DMA_CNT_7               0xCE

#define DMA_PAGE_0              0x87	/* DMA page registers */
#define DMA_PAGE_1              0x83
#define DMA_PAGE_2              0x81
#define DMA_PAGE_3              0x82
#define DMA_PAGE_5              0x8B
#define DMA_PAGE_6              0x89
#define DMA_PAGE_7              0x8A

#define DMA_MODE_READ	0x44	/* I/O to memory, no autoinit, increment, single mode */
#define DMA_MODE_WRITE	0x48	/* memory to I/O, no autoinit, increment, single mode */
#define DMA_MODE_CASCADE 0xC0	/* pass thru DREQ->HRQ, DACK<-HLDA only */

/* These are in dma.c */
extern void enable_dma(unsigned char);
extern void disable_dma(unsigned char);
extern void clear_dma_ff(unsigned char);
extern void set_dma_mode(unsigned char,unsigned char);
extern void set_dma_page(unsigned char,unsigned char);
extern void set_dma_addr(unsigned char,unsigned long);
extern void set_dma_count(unsigned char,unsigned int);
extern int get_dma_residue(unsigned char);
extern int request_dma(unsigned char,const char *);
extern void free_dma(unsigned char);

#endif
