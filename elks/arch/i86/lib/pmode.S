/*
 * 286/386+ real -> protected mode switch and auxillary routines.
 *
 * Called as last step in gdt_init(), after GDT initialization. enable_protected_mode()
 * is passed the two lgdt/lidt operands, which are the physical addresses of
 * the GDT and IDT.
 * The GDT kernel selectors used are:
 *     SEL_KCODE = GDT[1] kernel code, base = kernel_cs << 4
 *     SEL_KDATA = GDT[2] kernel data, base = kernel_ds << 4
 * Both bases equal the kernel's real-mode segments, so the same offsets resolve
 * to the same physical addresses across the switch (CS:IP and SS:SP stay valid).
 *
 * NOTE: enters PM with interrupts OFF.  gdt_init() has filled the IDT:
 * every vector points at pm_fault_stub (report on COM1 + halt) until irq_init()
 * installs the real IRQ/syscall handlers.  ftext has its own descriptor
 * and far-call selectors.
 */
#include <arch/segment.h>

        .arch   i286, nojumps
        .code16
        .text
        .global enable_protected_mode

// void pm_enable(struct dtr *gdtr, struct dtr *idtr);
enable_protected_mode:
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

        ljmp    $SEL_KCODE,$pm_cs // load CS = kernel code selector, flush prefetch
pm_cs:
        mov     $SEL_KDATA,%ax  // kernel data selector
        mov     %ax,%ds
        mov     %ax,%es
        mov     %ax,%ss         // SS base = kernel_ds<<4 (same phys as real-mode SS)

        mov     $'P',%al        // bring-up: signal successful PM entry on COM1
        call    pm_putc_ser

        pop     %bp
        ret                     // return (in PM) to gdt_init -> crt0 -> start_kernel

// Exception/fault catch + reporter
// gdt_init() points IDT 0..31 at pm_flt_base + vec*8 and 32..255 at pm_flt_other.
// Each stub pushes its vector then jumps to the common reporter, which prints
// "EXC=<num>-<errcode>-<IP>-<CS>-<FLAGS>-<rest of stack[0..4]>" on COM1 and halts.
// <num> is the exception number: #DF=08, #TS=0A, #NP=0B, #SS=0C, #GP=0D.
// Faulting CS will be known selector number: CODE 0008, DATA 0010, FTEXT 0018.

        .balign 8
        .global pm_flt_base
pm_flt_base:
        .set    fltv, 0
        .rept   32
        pushw   $fltv
        jmp     pm_fault_common
        .balign 8, 0x90         // each stub occupies an 8-byte slot
        .set    fltv, fltv + 1
        .endr

        .global pm_flt_other
pm_flt_other:
        pushw   $0x00FF         // use 00FF to mark vectors >= 32
pm_fault_common:
        cli
        mov     %sp,%bp         // bp -> [vec][ERRCODE][IP][CS][FLAGS][...]
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
        movw    (%bp),%ax       // vector
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    2(%bp),%ax      // errcode
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    4(%bp),%ax      // IP
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    6(%bp),%ax      // CS
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    8(%bp),%ax      // FLAGS
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    10(%bp),%ax     // stack 0
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    12(%bp),%ax     // stack 1
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    14(%bp),%ax     // stack 2
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    16(%bp),%ax     // stack 3
        call    pm_puthex
        mov     $'-',%al
        call    pm_putc_ser
        movw    18(%bp),%ax     // stack 4
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
