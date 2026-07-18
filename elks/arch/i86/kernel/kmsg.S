/*
 * kmsg.S - Kernel message ring buffer in far memory
 *
 * The ring buffer is placed below the EBDA, carved from
 * memend before mm_init(). This avoids consuming kernel data segment
 * space (no static 2KB buffer in BSS).
 *
 * void kmsg_init(seg_t seg, unsigned int data_size);
 * void kmsg_enqueue(seg_t seg, unsigned char ch);
 *
 * Both functions take the segment address as an explicit parameter.
 *
 * Reentrant-safe: uses cli/sti around critical section.
 * Minimal stack usage: only saves BP, DS, BX, SI (4 words).
 * Called from kputchar() which can be invoked from IRQ context.
 */

#include <linuxmt/config.h>

#ifdef CONFIG_CHAR_DEV_KMSG

	.code16
    .text

    /*
     * kmsg_init - initialize ring buffer header in far memory
     *
     * C prototype: void kmsg_init(seg_t seg, unsigned int data_size)
     * Parameters on stack: [ret][seg][data_size]
     */
    .globl kmsg_init
kmsg_init:
    push %bp
    mov %sp,%bp
    push %ds
    push %bx

    mov 4(%bp),%ax     /* AX = segment parameter */
    or %ax,%ax
    jz 9f

    mov %ax,%ds
    xor %bx,%bx

    movw $0,0(%bx)     /* head = 0 */
    movw $0,2(%bx)     /* tail = 0 */
    movw $0,4(%bx)     /* count = 0 */
    mov 6(%bp),%ax     /* AX = data_size argument */
    mov %ax,6(%bx)     /* size = data_size */

9:  pop %bx
    pop %ds
    pop %bp
    ret

    /*
     * kmsg_enqueue - enqueue one character into the far-memory ring buffer
     *
     * C prototype: void kmsg_enqueue(seg_t seg, unsigned char ch)
     * Parameters on stack: [ret][seg][ch]
     *
     * Ring buffer header layout (all at offsets from seg:0):
     *   0: head (unsigned int)   - next write position
     *   2: tail (unsigned int)   - next read position
     *   4: count (unsigned int)  - bytes available
     *   6: size (unsigned int)   - data[] capacity
     *   8: data[0..size-1]      - circular message data area
     */
    .globl kmsg_enqueue
kmsg_enqueue:
    push %bp
    mov %sp,%bp
    push %ds
    push %bx
    push %si

    /*
     * Read segment from stack parameter.
     * If 0 (early boot, before main.c reserves the buffer), skip.
     */
    mov 4(%bp),%ax     /* AX = segment parameter */
    or %ax,%ax
    jz 9f               /* skip if seg == 0 */

    mov %ax,%ds         /* DS = buffer segment */

    xor %bx,%bx         /* BX = 0 (base offset for header access) */

    cli                 /* protect from reentrancy - printk can be called from IRQ */

    /* Load head */
    mov 0(%bx),%si      /* SI = head */

    /* Write character to data[head] */
    mov 6(%bp),%al      /* AL = character (low byte of word arg) */
    add $8,%si          /* SI = offset of data[head] */
    mov %al,(%si)       /* data[head] = ch */
    sub $8,%si          /* SI = head value again */

    /* Advance head with wrap */
    inc %si             /* head++ */
    cmp 6(%bx),%si      /* head >= size? */
    jl 1f
    xor %si,%si         /* wrap to 0 */
1:  mov %si,0(%bx)      /* store new head */

    /* Increment count */
    incw 4(%bx)         /* count++ */

    /* Check if buffer is full */
    mov 4(%bx),%si      /* SI = count */
    cmp 6(%bx),%si      /* count > size? */
    jle 2f

    /* Buffer full: clamp count and advance tail */
    mov 6(%bx),%si
    mov %si,4(%bx)      /* count = size */

    mov 2(%bx),%si      /* SI = tail */
    inc %si             /* tail++ */
    cmp 6(%bx),%si      /* tail >= size? */
    jl 3f
    xor %si,%si         /* wrap to 0 */
3:  mov %si,2(%bx)      /* store new tail */

2:  sti

9:  pop %si
    pop %bx
    pop %ds
    pop %bp
    ret

#endif /* CONFIG_CHAR_DEV_KMSG */
