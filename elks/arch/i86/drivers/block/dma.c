/*
 * linux/kernel/dma.c: A DMA channel allocator. Inspired by linux/kernel/irq.c.
 *
 * Written by Hennus Bergman, 1992.
 *
 * 1994/12/26: Changes by Alex Nash to fix a minor bug in /proc/dma.
 *   In the previous version the reported device could end up being wrong,
 *   if a device requested a DMA channel that was already in use.
 *   [It also happened to remove the sizeof(char *) == sizeof(int)
 *   assumption introduced because of those /proc/dma patches. -- Hennus]
 */

#include <linuxmt/config.h>

#ifdef CONFIG_BLK_DEV_FD

#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <arch/dma.h>
#include <arch/system.h>

/* A note on resource allocation:
 *
 * All drivers needing DMA channels, should allocate and release them
 * through the public routines `request_dma()' and `free_dma()'.
 *
 * In order to avoid problems, all processes should allocate resources in
 * the same sequence and release them in the reverse order.
 *
 * So, when allocating DMAs and IRQs, first allocate the IRQ, then the DMA.
 * When releasing them, first release the DMA, then release the IRQ.
 * If you don't, you may cause allocation requests to fail unnecessarily.
 * This doesn't really matter now, but it will once we get real semaphores
 * in the kernel.
 */

/* Channel n is busy iff dma_chan_busy[n].lock != 0.
 * DMA0 used to be reserved for DRAM refresh, but apparently not any more...
 * DMA4 is reserved for cascading.
 */

struct dma_chan {
    int lock;
    const char *device_id;
};

static struct dma_chan dma_chan_busy[MAX_DMA_CHANNELS] = {
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {1, "cascade"},
    {0, 0},
    {0, 0},
    {0, 0}
};

#define xchg(op, arg) \
( {			   \
	typeof(arg) __ret; \
	__ret = *op;	   \
	*op = arg;	   \
	__ret;		   \
} )

int request_dma(unsigned char dma, const char *device)
{
    if (dma >= MAX_DMA_CHANNELS)
	return -EINVAL;

    if (xchg(&dma_chan_busy[dma].lock, 1) != 0)
	return -EBUSY;

    dma_chan_busy[dma].device_id = device;

    /* old flag was 0, now contains 1 to indicate busy */
    return 0;
}				/* request_dma */

void free_dma(unsigned char dma)
{
    if (dma >= MAX_DMA_CHANNELS)
	debug("Trying to free DMA%u\n", dma);
    else if (!xchg(&dma_chan_busy[dma].lock, 0)) {
	printk("Trying to free free DMA%u\n", dma);
} }				/* free_dma */

/* enable/disable a specific DMA channel */

void enable_dma(unsigned char dma)
{
    if (dma >= MAX_DMA_CHANNELS)
	debug("Trying to enable DMA%u\n", dma);
    else if (dma <= 3)
	dma_outb(dma, DMA1_MASK_REG);
    else
	dma_outb(dma & 3, DMA2_MASK_REG);
}

void disable_dma(unsigned char dma)
{
    if (dma >= MAX_DMA_CHANNELS)
	debug("Trying to disable DMA%u\n", dma);
    else if (dma <= 3)
	dma_outb(dma | 4, DMA1_MASK_REG);
    else
	dma_outb((dma & 3) | 4, DMA2_MASK_REG);
}

/* Clear the 'DMA Pointer Flip Flop'.
 * Write 0 for LSB/MSB, 1 for MSB/LSB access.
 * Use this once to initialize the FF to a known state.
 * After that, keep track of it. :-)
 * --- In order to do that, the DMA routines below should ---
 * --- only be used while interrupts are disabled! ---
 */

void clear_dma_ff(unsigned char dma)
{
    if (dma >= MAX_DMA_CHANNELS)
	debug("Trying to disable DMA%u\n", dma);
    else if (dma <= 3)
	dma_outb(0, DMA1_CLEAR_FF_REG);
    else
	dma_outb(0, DMA2_CLEAR_FF_REG);
}

/* set mode (above) for a specific DMA channel */

void set_dma_mode(unsigned char dma, unsigned char mode)
{
    if (dma >= MAX_DMA_CHANNELS)
	debug("Trying to disable DMA%u\n", dma);
    else if (dma <= 3)
	dma_outb(mode | dma, DMA1_MODE_REG);
    else
	dma_outb(mode | (dma & 3), DMA2_MODE_REG);
}

/* Set only the page register bits of the transfer address.
 * This is used for successive transfers when we know the contents of
 * the lower 16 bits of the DMA current address register, but a 64k
 * boundary may have been crossed.
 */

void set_dma_page(unsigned char dma, unsigned char page)
{
    switch (dma) {
    case 0:
	dma_outb(page, DMA_PAGE_0);
	break;
    case 1:
	dma_outb(page, DMA_PAGE_1);
	break;
    case 2:
	dma_outb(page, DMA_PAGE_2);
	break;
    case 3:
	dma_outb(page, DMA_PAGE_3);
	break;
    case 5:
	dma_outb(page & 0xfe, DMA_PAGE_5);
	break;
    case 6:
	dma_outb(page & 0xfe, DMA_PAGE_6);
	break;
    case 7:
	dma_outb(page & 0xfe, DMA_PAGE_7);
	break;
    }
}

/* Set transfer address & page bits for specific DMA channel.
 * Assumes dma flipflop is clear.
 */

void set_dma_addr(unsigned char dma, unsigned long addr)
{
    set_dma_page(dma, (long)addr >> 16);
    if (dma <= 3) {
	dma_outb(addr & 0xff, (dma << 1) + IO_DMA1_BASE);
	dma_outb((addr >> 8) & 0xff, (dma << 1) + IO_DMA1_BASE);
    } else {
	dma_outb((addr >> 1) & 0xff, ((dma & 3) << 2) + IO_DMA2_BASE);
	dma_outb((addr >> 9) & 0xff, ((dma & 3) << 2) + IO_DMA2_BASE);
    }
}

/* Set transfer size (max 64k for DMA1..3, 128k for DMA5..7) for
 * a specific DMA channel.
 * You must ensure the parameters are valid.
 * NOTE: from a manual: "the number of transfers is one more
 * than the initial word count"! This is taken into account.
 * Assumes dma flip-flop is clear.
 * NOTE 2: "count" represents _bytes_ and must be even for channels 5-7.
 */

void set_dma_count(unsigned char dma, unsigned int count)
{
    count--;
    if (dma <= 3) {
	dma_outb(count & 0xff, (dma << 1) + 1 + IO_DMA1_BASE);
	dma_outb((count >> 8) & 0xff, (dma << 1) + 1 + IO_DMA1_BASE);
    } else {
	dma_outb((count >> 1) & 0xff, ((dma & 3) << 2) + 2 + IO_DMA2_BASE);
	dma_outb((count >> 9) & 0xff, ((dma & 3) << 2) + 2 + IO_DMA2_BASE);
    }
}

#if UNUSED
/* Get DMA residue count. After a DMA transfer, this
 * should return zero. Reading this while a DMA transfer is
 * still in progress will return unpredictable results.
 * If called before the channel has been used, it may return 1.
 * Otherwise, it returns the number of _bytes_ left to transfer.
 *
 * Assumes DMA flip-flop is clear.
 */

int get_dma_residue(unsigned char dma)
{
    unsigned int io_port = (dma <= 3) ? (dma << 1) + 1 + IO_DMA1_BASE
				      : ((dma & 3) << 2) + 2 + IO_DMA2_BASE;

    /* using short to get 16-bit wrap around */
    unsigned short count = 1 + dma_inb(io_port);

    count += dma_inb(io_port) << 8;

    return (dma <= 3) ? count : (count << 1);
}

int get_dma_list(char *buf)
{
    int i, len = 0;

    for (i = 0; i < MAX_DMA_CHANNELS; i++)
	if (dma_chan_busy[i].lock)
	    len += sprintf(buf + len, "%2d: %s\n", i, dma_chan_busy[i].device_id);
    return len;
}
#endif

#endif
