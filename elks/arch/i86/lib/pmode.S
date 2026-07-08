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
        pushf                   // save interrupt status
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

        popf                    // restore interrupt status
        pop     %bp
        ret                     // return (in PM) to gdt_init -> crt0 -> start_kernel

// Exception/fault catch + reporter
// gdt_init() points IDT 0..31 at pm_fault_panic + vec*8 and the rest at pm_fault_panic2.
// Each stub pushes its vector then jumps to the common reporter, which prints
// "EXC=<vecnum>-<errcode>-<IP>-<CS>-<FLAGS>-<rest of stack[0..4]>" on COM1 and halts.
// <num> is the exception number: #DF=08, #TS=0A, #NP=0B, #SS=0C, #GP=0D.
// Faulting CS will be known selector number: CODE 0008, DATA 0010, FTEXT 0018.

        .balign 2
        .global pm_fault_vector
        .extern pm_exception_handler
pm_fault_vector:
        .set    fltv, 0
        .rept   32              // first 32 IDT entries report vector numbers 0..31
        pushw   $fltv
        call    pm_exception_handler
        nop; nop; nop;          // each entry is 5 bytes code + 3 bytes NOP
        .set    fltv, fltv + 1
        .endr
        pushw   $0x00FF         // remainder vectors >= 32 report vector 255
        call    pm_exception_handler

// send AL to COM1. Used as early_putchar before console init.
        .global putc_ser
putc_ser:
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
