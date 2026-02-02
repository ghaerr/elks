// Top half of fast serial interrupt handlers for ELKS
//
// Runs on any stack and skips ELKS _irqit stack switching overhead.
// Must run with interrupts disabled as could interrupt user, kernel or interrupt stack.
// Reads received byte into tty input queue at interrupt time.
// The bottom half (serial_bh) runs later, which processes queue and calls wake_up.
//
// 25 June 2020 Greg Haerr
//  1 Feb 2026 Rewritten completely in assembly
//
#include <linuxmt/config.h>
        .code16
        .text
#ifdef CONFIG_ARCH_IBMPC

        .global ports                   // struct serial_info ports[]
        .global asm_fast_com1           // entry points
        .global asm_fast_com2
        .global asm_fast_com3
        .global asm_fast_com4

// Original C handler rewritten in ASM:
//
// void rs_fast_com1(void)
// {
//     struct serial_info *sp = &ports[0];
//     struct ch_queue *q = &sp->tty->inq;
//     unsigned char c;
//
//     c = INB(sp->io + UART_RX);       // Read received data
//     if (q->len < q->size) {
//         q->base[q->head] = c;
//         if (++q->head >= q->size)
//             q->head = 0;
//         q->len++;
//     }
//     if (c == 03)                     // assumes VINTR = ^C and byte queued anyways
//         sp->intrchar = c;
// }

//
// Entry for ttyS0 top half interrupt handler, called by CALLF within dynamic handler
//
asm_fast_com1:
        push    %cx
        mov     $ports+0,%cx            // cx = &ports[0]
common_entry:
        push    %ax                     // save regs, uses 14 bytes of current stack
        push    %bx
        push    %dx
        push    %ds

        // Recover kernel data segment
        // Was pushed by the CALLF of the dynamic handler
        mov     %sp,%bx
        mov     %ss:12(%bx),%ds

        // translated C routine above
        // NOTE: uses hard-coded offsets from struct serial_info, tty and ch_queue
        push    %si
        push    %di
        mov     %cx,%bx                 // bx = sp = &ports[n]
        mov     (%bx),%si               // si = q = &sp->tty->inq
        mov     0x4(%bx),%dx            // dx = sp->io + UART_RX
        in      (%dx),%al
        mov     %al,%dl                 // dl = c = INB(sp->io+UART_RX)
        mov     (%si),%ax               // ax = q->len
        cmp     0x2(%si),%ax            // if (ax >= q->size)
        jge     2f
        mov     0x4(%si),%bx            // bx = q->head
        mov     0x8(%si),%di            // di = q->base
        mov     %dl,(%bx,%di)           // q->base[q->head] = c
        mov     0x4(%si),%ax            // ax = q->head
        inc     %ax
        mov     %ax,0x4(%si)            // ++q->head
        cmp     0x2(%si),%ax            // if (q->size < q->head)
        jl      1f
        movw    $0x0,0x4(%si)           // q->head = 0
1:      incw    (%si)
2:      cmp     $0x3,%dl                // if (c == 03)
        jne     3f
        mov     %cx,%bx                 // bx = sp
        movw    $0x3,2(%bx)             // sp->intrchar = c
3:      pop     %di
        pop     %si

        mov     $0x20,%al               // EOI on primary controller
        out     %al,$0x20
        pop     %ds                     // restore regs
        pop     %dx
        pop     %bx
        pop     %ax
        pop     %cx
        add     $4,%sp                  // skip the trampoline DS:*irq
        iret

//
// Entry for ttyS1 top half interrupt handler, called by CALLF within dynamic handler
//
asm_fast_com2:
        push    %cx
        mov     $ports+16,%cx           // cx = &ports[1]
        jmp     common_entry

//
// Entry for ttyS2 top half interrupt handler, called by CALLF within dynamic handler
//
asm_fast_com3:
        push    %cx
        mov     $ports+32,%cx           // cx = &ports[2]
        jmp     common_entry

//
// Entry for ttyS3 top half interrupt handler, called by CALLF within dynamic handler
//
asm_fast_com4:
        push    %cx
        mov     $ports+48,%cx           // cx = &ports[3]
        jmp     common_entry
#endif

#ifdef CONFIG_FAST_IRQ1_NECV25
//
// Entry for console-serial-necv25 top half interrupt handler,
//      called by CALLF within dynamic handler
//
    .extern sercon_fast_irq1_necv25
    .global asm_fast_irq1_necv25
asm_fast_irq1_necv25:
    push    %ax         // save regs, uses 18 bytes of current stack
    push    %bx
    push    %cx
    push    %dx
    push    %ds

    // Recover kernel data segment
    // Was pushed by the CALLF of the dynamic handler
    mov %sp,%bx
    mov %ss:12(%bx),%ds

    call    sercon_fast_irq1_necv25 // call special 2nd part of top half C handler
                                    // which doesn't use any SS/SP/BP addressing

    .word   0x920f      // NEC V25 specific FINT (End Of Interrupt) instruction

    pop     %ds         // restore regs
    pop     %dx
    pop     %cx
    pop     %bx
    pop     %ax
    add     $4,%sp      // skip the trampoline DS:*irq
    iret
#endif

#ifdef CONFIG_FAST_IRQ2_NECV25
//
// Entry for console-serial-necv25 top half interrupt handler,
//      called by CALLF within dynamic handler
//
    .extern sercon_fast_irq2_necv25
    .global asm_fast_irq2_necv25
asm_fast_irq2_necv25:
    push    %ax         // save regs, uses 18 bytes of current stack
    push    %bx
    push    %cx
    push    %dx
    push    %ds

    // Recover kernel data segment
    // Was pushed by the CALLF of the dynamic handler
    mov %sp,%bx
    mov %ss:12(%bx),%ds

    call    sercon_fast_irq2_necv25 // call special 2nd part of top half C handler
                                    // which doesn't use any SS/SP/BP addressing

    .word   0x920f      // NEC V25 specific FINT (End Of Interrupt) instruction

    pop     %ds         // restore regs
    pop     %dx
    pop     %cx
    pop     %bx
    pop     %ax
    add     $4,%sp      // skip the trampoline DS:*irq
    iret
#endif
