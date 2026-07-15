#ifndef __LINUXMT_KMSG_H
#define __LINUXMT_KMSG_H

/*
 * Kernel message ring buffer in far memory (top of physical RAM).
 *
 * The buffer header is at the start of the reserved region,
 * followed by the circular data area. The kernel writes via
 * kmsg_enqueue() (ASM, reentrant-safe). User-space reads
 * via /dev/kmem using the segment address returned by
 * MEM_GETKMSG ioctl.
 *
 * Buffer layout in far memory:
 *   offset 0: head (unsigned int)  - write position into data[]
 *   offset 2: tail (unsigned int)  - read position into data[]
 *   offset 4: count (unsigned int) - bytes available to read
 *   offset 6: size (unsigned int)  - data[] capacity (power of 2)
 *   offset 8: data[0..size-1]     - circular message buffer
 */

struct kmsg_hdr {
    volatile unsigned int k_head;
    volatile unsigned int k_tail;
    volatile unsigned int k_count;
    unsigned int k_size;
};

#define KMSG_DATA_OFF   8   /* offset of data[] from start of buffer */
#define KMSG_BUF_SIZE   2048 /* default buffer size, overridable via /bootopts dmesg= */

#ifdef __KERNEL__

extern volatile seg_t kmsg_seg;              /* segment address of ring buffer (0 = disabled) */
extern unsigned int kmsg_buf_size;  /* requested buffer size from /bootopts dmesg= */
extern void kmsg_init(seg_t seg, unsigned int data_size); /* init header in far memory */
extern void kmsg_enqueue(seg_t seg, unsigned char ch); /* enqueue one char (ASM, reentrant-safe) */

#endif /* __KERNEL__ */

#endif /* __LINUXMT_KMSG_H */
