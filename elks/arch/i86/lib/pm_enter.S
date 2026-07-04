/*
 * 80286 real -> protected mode switch.  (phase 4 of the 286 HAL)
 *
 * Called once from crt0.S (CONFIG_286_PMODE) after kernel_cs/kernel_ds are
 * saved and the temporary stack is set, just before start_kernel.  The GDT
 * has already been filled by gdt_init() (pm286.c), which passes the lgdt/lidt
 * operands.  Kernel selectors (see <arch/seg286.h>):
 *     0x08 = GDT[1] kernel code, base = kernel_cs << 4
 *     0x10 = GDT[2] kernel data, base = kernel_ds << 4
 * Both bases equal the kernel's real-mode segments, so the same offsets resolve
 * to the same physical addresses across the switch (CS:IP and SS:SP stay valid).
 *
 * NOTE: enters PM with interrupts OFF.  gdt_init() has filled the IDT (phase 5a):
 * every vector points at pm_fault_stub (report on COM1 + halt) until irq_init()
 * installs the real IRQ/syscall handlers (phase 5b).  ftext has its own descriptor
 * and far-call selectors (phase 4b).
 */
        .arch   i286, nojumps
        .code16
        .text
        .global pm_switch

// void pm_switch(struct dtr *gdtr, struct dtr *idtr);   /* near cdecl */
pm_switch:
        push    %bp
        mov     %sp,%bp
        cli                     // no interrupts during the transition

        mov     4(%bp),%bx      // bx -> gdtr { word limit; long base }
        lgdt    (%bx)
        mov     6(%bp),%bx      // bx -> idtr (empty for now)
        lidt    (%bx)

        smsw    %ax             // 286: enter PM via LMSW (no mov-to-CR0)
        or      $1,%ax          // set PE
        lmsw    %ax             // *** now in protected mode ***

        ljmp    $0x08,$pm_cs    // load CS = kernel code selector, flush prefetch
pm_cs:
        mov     $0x10,%ax       // kernel data selector
        mov     %ax,%ds
        mov     %ax,%es
        mov     %ax,%ss         // SS base = kernel_ds<<4 (same phys as real-mode SS)

        mov     $'P',%al        // bring-up: signal successful PM entry on COM1
        call    pm_putc_ser

        pop     %bp
        ret                     // return (in PM) to gdt_init -> crt0 -> start_kernel

// ---- exception/fault catch + reporter (phase 8 bring-up) ----
// gdt_init() points IDT 0..31 at pm_flt_base + vec*8 and 32..255 at pm_flt_other.
// Each stub pushes its vector then jumps to the common reporter, which prints
// "EXC=<vec>-<w0>-<w1>-<w2>-<w3>" (vector + top stack words) on COM1 and halts.
// From <vec> = which exception; from the words = faulting CS:IP (CS is a known
// selector 0008/0010/0018), shifted one word if an error code was pushed
// (#DF=8 #TS=10 #NP=11 #SS=12 #GP=13).
        .balign 8
        .global pm_flt_base
pm_flt_base:
        .set    fltv, 0
        .rept   32
        pushw   $fltv
        jmp     pm_fault_common
        .balign 8, 0x90                 // each stub occupies an 8-byte slot
        .set    fltv, fltv + 1
        .endr

        .global pm_flt_other
pm_flt_other:
        pushw   $0x00FF                 // marker: vector >= 32 (shouldn't fire pre-irq_init)
        jmp     pm_fault_common

pm_fault_common:
        cli
        mov     %sp,%bp                 // bp -> [vec][..][IP][CS][FLAGS] (SS-relative)
        mov     $0x0d,%al
        call    pm_putc_ser
        mov     $0x0a,%al
        call    pm_putc_ser
        mov     $'E',%al
        call    pm_putc_ser
        mov     $'X',%al
        call    pm_putc_ser
        mov     $'C',%al
        call    pm_putc_ser
        mov     $'=',%al
        call    pm_putc_ser
        movw    (%bp),%ax               // vector
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    2(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    4(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    6(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    8(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    10(%bp),%ax     // (for fmemcpyb: caller return address)
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    12(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    14(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    16(%bp),%ax
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    18(%bp),%ax
        call    pm_puthex
        mov     $0x0d,%al
        call    pm_putc_ser
        mov     $0x0a,%al
        call    pm_putc_ser
9:      hlt
        jmp     9b

// AX -> 4 hex digits on COM1 (high nibble first)
pm_puthex:
        push    %cx
        push    %bx
        mov     %ax,%bx
        mov     $4,%cx
1:      rol     $4,%bx
        mov     %bx,%ax
        and     $0x0f,%ax
        cmp     $10,%ax
        jb      2f
        add     $7,%ax
2:      add     $0x30,%ax
        call    pm_putc_ser
        loop    1b
        pop     %bx
        pop     %cx
        ret

// AL = char -> COM1 THR (0x3F8), polled.  PM ring0 port I/O; clobbers nothing.
// Also called from crt0.S early_putchar in PM.
        .global pm_putc_ser
pm_putc_ser:
        push    %ax
        push    %dx
        mov     %al,%ah
8:      mov     $0x3FD,%dx      // LSR
        inb     %dx,%al
        test    $0x20,%al       // THR empty?
        jz      8b
        mov     $0x3F8,%dx      // THR
        mov     %ah,%al
        outb    %al,%dx
        pop     %dx
        pop     %ax
        ret
