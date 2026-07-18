// PCI configuration space access for the PCnet driver (pcnet.c)
//
// Uses configuration mechanism #1 (I/O ports 0xCF8/0xCFC), which requires
// 32-bit port I/O -> 80386+ only.  The caller (pcnet_pci_find) guards on
// arch_cpu >= CPU_80386 before calling these.  gas emits the 0x66 operand-
// size prefixes automatically under .arch i386 / .code16, the same idiom as
// arch/i86/lib/unreal.S.  Only EAX/EDX (caller-saved AX/DX) are touched;
// BP is saved.
//
// These are FAR functions (declared __far in pcnet.c) placed in
// .fartext.init: the INITPROC caller pcnet_pci_find lives in a different
// segment, so the compiler emits a far call (lcall) and expects a far
// return (lret).  Far frame after 'push bp; mov sp,bp':
//   bp+0 saved bp, bp+2 ret IP, bp+4 ret CS, bp+6 first 16-bit arg.

    .arch i386,nojumps
    .code16
    .section .fartext.init,"ax",@progbits

// build config address in EAX from devfn_reg at BP+6:
//   0x80000000 | (dev<<11) | (func<<8) | (reg & 0xFC)   (bus 0)
.macro CFGADDR
    movzwl  6(%bp),%eax
    andl    $0x0000FFFC,%eax        // dword-align reg, keep dev/func/reg
    orl     $0x80000000,%eax        // enable bit, bus 0
    movw    $0xCF8,%dx
    outl    %eax,%dx
.endm

// unsigned long __far pci_cfg_read(unsigned int devfn_reg)  -> DX:AX
    .global pci_cfg_read
pci_cfg_read:
    push    %bp
    mov     %sp,%bp
    CFGADDR
    movw    $0xCFC,%dx
    inl     %dx,%eax
    movl    %eax,%edx
    shrl    $16,%edx               // DX = high 16, AX = low 16
    movzwl  %ax,%eax               // clear upper EAX (32-bit ops must not
    movzwl  %dx,%edx               // leave dirty high halves for 16-bit code)
    pop     %bp
    lret

// void __far pci_cfg_write(unsigned int devfn_reg, unsigned int val)
    .global pci_cfg_write
pci_cfg_write:
    push    %bp
    mov     %sp,%bp
    CFGADDR
    movzwl  8(%bp),%eax            // val (2nd arg)
    movw    $0xCFC,%dx
    outl    %eax,%dx
    xorl    %eax,%eax              // clear dirty EAX upper half
    pop     %bp
    lret
