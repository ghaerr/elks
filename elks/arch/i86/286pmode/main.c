/*
 * arch/i86/286pmode/main.c
 *
 * main source file for the 286pmode ELKS kernel extender
 */

#include "descriptor.h"
#include "tss.h"

#define PMODE_CODESEL 0x08
#define PMODE_DATASEL 0x10
#define PMODE_SCRNSEL 0x18
#define PMODE_TEMPSEL 0x20

#define PMODE_TSSSEL 0x28
#define PMODE_LDTSEL 0x30

#define ELKS_CODESEL 0x39
#define ELKS_DATASEL 0x41

#define GATE_INTERRUPT 0x86
#define GATE_TRAP      0x87

extern unsigned short _endtext;
extern unsigned short _enddata;
extern unsigned short _endbss;
extern unsigned short _elksheader;
extern unsigned char arch_cpu;
extern int bootstack; /* this really wants to be extern void, but bcc sucks. */

unsigned short realmode_ds; /* used for calculating the address of the IDT */

/* 0x20 processor exceptions, 0x10 hardware ints */
struct descriptor idt[0x30];

#define IDT_LIMIT 0x17f

struct descriptor initial_gdt[9] = {
    {0x0000, 0x0000, 0x00, 0x00, 0x0000}, /* NULL segment */
    {0xffff, 0x0020, 0x01, 0x9b, 0x0000}, /* ring 0 code segment (ours) */
    {0xffff, 0x0020, 0x01, 0x93, 0x0000}, /* ring 0 data segment (ours) */
    {0xffff, 0x0000, 0x00, 0x93, 0x0000}, /* ring 0 screen data segment */
    {0xffff, 0x0000, 0x00, 0x93, 0x0000}, /* ring 0 temporary data segment */
    {0x002b, 0x0000, 0x00, 0x81, 0x0000}, /* initial TSS */
    {0x0000, 0x0000, 0x00, 0x82, 0x0000}, /* 0-length LDT for TSS */
    {0x0000, 0x0000, 0x00, 0xbb, 0x0000}, /* ring 1 code segment (ELKSs) */
    {0xffff, 0x0000, 0x00, 0xb3, 0x0000}, /* ring 1 data segment (ELKSs) */
};

#define GDT_LIMIT 0x47

/* most of these fields can be nil without affecting anything. */
struct tss initial_tss = {
    0x0000, /* backlink */
    0x0000, PMODE_DATASEL, /* cpl0 sp, cpl0 ss */
    0x0000, 0x0000, /* cpl1 sp, cpl1 ss */
    0x0000, 0x0000, /* cpl2 sp, cpl2 ss */
    0x0000, 0x0000, /* ip, flags */
    0x0000, 0x0000, 0x0000, 0x0000, /* ax, cx, dx, bx */
    0x0000, 0x0000, 0x0000, 0x0000, /* sp, bp, si, di */
    0x0000, 0x0000, 0x0000, 0x0000, /* es, cs, ss, ds */
    PMODE_LDTSEL, /* ldt */
};

char hexchars[0x10] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
};

/* these all want to be extern void, but you saw my previous comment. */
extern int exc_00; extern int exc_01; extern int exc_02; extern int exc_03;
extern int exc_04; extern int exc_05; extern int exc_06; extern int exc_07;
extern int exc_08; extern int exc_09; extern int exc_0a; extern int exc_0b;
extern int exc_0c; extern int exc_0d; extern int exc_0e; extern int exc_10;
extern int exc_11; extern int exc_xx;

extern int irq_0; extern int irq_1; extern int irq_2; extern int irq_3;
extern int irq_4; extern int irq_5; extern int irq_6; extern int irq_7;
extern int irq_8; extern int irq_9; extern int irq_a; extern int irq_b;
extern int irq_c; extern int irq_d; extern int irq_e; extern int irq_f;

unsigned short irqhandlers[0x30] = {
    &exc_00, &exc_01, &exc_02, &exc_03, &exc_04, &exc_05, &exc_06, &exc_07,
    &exc_08, &exc_09, &exc_0a, &exc_0b, &exc_0c, &exc_0d, &exc_0e, &exc_xx,
    &exc_10, &exc_11, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx,
    &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx,
    &irq_0, &irq_1, &irq_2, &irq_3, &irq_4, &irq_5, &irq_6, &irq_7,
    &irq_8, &irq_9, &irq_a, &irq_b, &irq_c, &irq_d, &irq_e, &irq_f,
};

void cli()
{
#asm
    cli
#endasm
}

void sti()
{
#asm
    sti
#endasm
}

unsigned short get_ds()
{
#asm
    mov ax, ds
#endasm
}

unsigned short get_flags()
{
#asm
    pushf
    pop ax
#endasm
}

void set_flags(flags)
     unsigned short flags;
{
#asm
    pop bx
    pop ax
    push ax
    push bx
    push ax
    popf
#endasm
}

void load_gdtr()
{
#asm
    push es
    mov ax, #0xb000
    mov es, ax
    mov ax, #0x0736
    seg es
    mov [0x24], ax
    pop es
	
    push bp
    mov bp, sp
    mov ax, #_initial_gdt
    xor bx, bx
    mov dx, ds
    shl dx, #4
    add ax, dx
    adc bx, #0
    mov dx, ds
    shr dx, #0xc
    add bx, dx

    mov cx, #GDT_LIMIT
    push bx
    push ax
    push cx
    lgdt [bp-6]

#if 0
    push es
    push di
    mov ax, #0xb000
    mov es, ax
    mov di, #0xa0
    cld
    mov ah, #0x07
    mov dx, ds
    xor bx, bx
    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    mov al, #0x20
    stosw

    mov dx, [bp-2]
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    mov dx, [bp-4]

    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    mov al, #0x20
    stosw

    mov dx, #_initial_gdt

    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    pop di
    pop es
/*     db 0xeb, 0xfe */
#endif
    add sp, #6
    pop bp

    push es
    mov ax, #0xb000
    mov es, ax
    mov ax, #0x0734
    seg es
    mov [0x28], ax
    pop es
#endasm
}

void load_idtr()
{
#asm
    push bp
    mov bp, sp
    mov ax, #_idt
    xor bx, bx
    mov dx, _realmode_ds
    shl dx, #4
    add ax, dx
    adc bx, #0
    mov dx, _realmode_ds
    shr dx, #0xc
    add bx, dx
    mov cx, #IDT_LIMIT
    push bx
    push ax
    push cx
    lidt [bp-6]

#if 0
    push es
    push di
    mov ax, #PMODE_SCRNSEL
    mov es, ax
    mov di, #0x140
    cld
    mov ah, #0x07
    mov dx, _realmode_ds
    xor bx, bx
    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    mov al, #0x20
    stosw

    mov dx, [bp-2]
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    mov dx, [bp-4]

    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    mov al, #0x20
    stosw

    mov dx, #_idt

    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    pop di
    pop es
/*     db 0xeb, 0xfe */
#endif
    add sp, #6
    pop bp
#endasm
}

void load_tr()
{
#asm
    mov ax, #PMODE_TSSSEL
    ltr ax
#endasm
}

void set_idt_entry(interrupt, cs, ip, type)
     unsigned short interrupt, cs, ip, type;
{
    unsigned short flags;

    flags = get_flags();
    cli();
    
    idt[interrupt].limit = ip;
    idt[interrupt].baseaddr0 = cs;
    idt[interrupt].flags = type;
    idt[interrupt].baseaddr1 = 0;
    idt[interrupt].reserved = 0;

    set_flags(flags);
}

void idt_init()
{
    int i;

    for (i = 0; i < 0x30; i++) {
        set_idt_entry(i, PMODE_CODESEL, irqhandlers[i], GATE_INTERRUPT);
    }
}

char irq_mesg[] = "received irq ";

void do_irq(irqnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax)
     unsigned short irqnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax;
{
#asm
    push bp
    mov bp, sp

    push es
    mov ax, #PMODE_SCRNSEL
    mov es, ax
    mov di, #0x280
    cld
    mov ah, #0x07

    mov si, #_irq_mesg
do_irq_print_loop:
    lodsb
    or al, al
    jz do_irq_done_print
    stosw
    jmp do_irq_print_loop
do_irq_done_print:

    xor bh, bh
    mov dx, [bp+4]

    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    pop es
    db 0xeb, 0xfe

    pop bp
#endasm
}

char exc_mesg[] = "recieved exception ";

void do_exc(excnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax)
     unsigned short excnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax;
{
#asm
    push bp
    mov bp, sp

    push es
    mov ax, #PMODE_SCRNSEL
    mov es, ax
    mov di, #0x280
    cld
    mov ah, #0x07

    mov si, #_exc_mesg
do_exc_print_loop:
    lodsb
    or al, al
    jz do_exc_done_print
    stosw
    jmp do_exc_print_loop
do_exc_done_print:

    xor bh, bh
    mov dx, [bp+4]

    mov bl, dh
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al, [bx+_hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al, [bx+_hexchars]
    stosw

    pop es
    db 0xeb, 0xfe

    pop bp
#endasm
}

void revector_8259s(where)
     unsigned short where;
{
#asm
   pop dx /* cheap trick to get the parameter into bx */
   pop bx
   push bx
   push dx
   mov al, #0x11
   out 0x20, al
   db 0xeb, 0x00 /* this code may not require quite so much delay for I/O, */
   db 0xeb, 0x00 /* any opinions? */
   db 0xeb, 0x00
   mov al, bl
   out 0x21, al
   db 0xeb, 0x00
   db 0xeb, 0x00
   db 0xeb, 0x00
   mov al, #0x4
   out 0x21, al
   db 0xeb, 0x00
   db 0xeb, 0x00
   db 0xeb, 0x00
   mov al, #0x1
   out 0x21, al
   db 0xeb, 0x00
   db 0xeb, 0x00
   db 0xeb, 0x00
   mov al, #0x11
   out 0xa0, al
   db 0xeb, 0x00
   db 0xeb, 0x00
   db 0xeb, 0x00
   mov al, bh
   out 0xa1, al
   db 0xeb, 0x00
   db 0xeb, 0x00
   db 0xeb, 0x00
   mov al, #0x2
   out 0xa1, al
   db 0xeb, 0x00
   db 0xeb, 0x00
   db 0xeb, 0x00
   mov al, #0x1
   out 0xa1, al
#endasm
}

void disable_irqs()
{
#asm
    mov al, #0xff
    out 0x21, al
    out 0xa1, al
#endasm
}

void fix_flags_for_pmode()
{
    if (arch_cpu > 6) {
#asm
    pushfd
    pop eax
    and eax, #0x00000fff
    push eax
    popfd
#endasm
    } else {
#asm
    pushf
    pop ax
    and ax, #0x0fff
    push ax
    popf
#endasm
    }
}

void switch_to_pmode()
{
#asm
    call _fix_flags_for_pmode
    smsw ax
    or al, #1
    lmsw ax
    db 0xea
    dw stpm_pmode
    dw PMODE_CODESEL
stpm_pmode:
    mov ax, #PMODE_DATASEL
    mov ss, ax
    mov ds, ax
    mov es, ax
#endasm
}

/* FIXME: I don't trust this compiler to generate accurate code for this. */
void init_ksegs(textlen)
     unsigned short textlen;
{
    initial_gdt[7].baseaddr0 = ((long)_elksheader + 2) << 4;
    initial_gdt[7].baseaddr1 = (((long)_elksheader + 2) >> 0xc) &0xff;
    initial_gdt[7].limit = textlen;

    initial_gdt[8].baseaddr0 = (((_elksheader + 2) << 4) + textlen) & 0xffff;
    initial_gdt[8].baseaddr1 = ((((_elksheader + 2) << 4) + textlen) >> 0xc) & 0xff;
}

void bogus_magic()
{
#asm
    mov ax, #PMODE_SCRNSEL
    mov es, ax
    mov ax, #0x0736
    mov di, #0x580
    mov cx, #42
    repz
    stosw
    dw 0xfeeb
#endasm
}

void boot_kernel()
{
    initial_gdt[4].baseaddr0 = _elksheader << 4;
    initial_gdt[4].baseaddr1 = (_elksheader >> 0xc) & 0xff;

#asm
    mov ax, #PMODE_TEMPSEL
    mov es, ax
    seg es
    cmp [0], #0x0301
    jz boot_kernel_good_sig
    call _bogus_magic
boot_kernel_good_sig:
    seg es
    mov dx, [8]
    mov ax, ds
    mov es, ax
    push dx
    call _init_ksegs
    pop dx
    mov ax, #PMODE_TEMPSEL
    mov ds, ax
    mov bx, [8]
    mov si, [12]
    mov dx, [16]
    mov ax, #ELKS_DATASEL
    mov ds, ax
    mov es, ax
    push #ELKS_DATASEL
    push #0xfffe
    push #ELKS_CODESEL
    push #0x0000
    retf
#endasm
}

void pmodekern_init()
{
    realmode_ds = get_ds();
    
    initial_gdt[1].limit = _endtext;
    initial_gdt[2].limit = _enddata + _endbss;
    initial_gdt[2].baseaddr0 += _endtext;
    load_gdtr();

    cli();
    switch_to_pmode();
    
    initial_gdt[3].baseaddr0 = 0x0000;
    initial_gdt[3].baseaddr1 = 0x0b;
    initial_gdt[3].limit = 0xffff;

    revector_8259s(0x2820);
    
    idt_init();

    load_idtr();

    disable_irqs();
	
    sti();
#if 1
    initial_tss.cpl0_sp = (unsigned short) &bootstack;
    initial_gdt[4].baseaddr0 = initial_gdt[2].baseaddr0;
    initial_gdt[4].baseaddr1 = initial_gdt[2].baseaddr1;
    /* FIXME: should carry into baseaddr1 */
    initial_gdt[4].baseaddr0 += (unsigned short) &initial_tss;

    load_tr();
#endif

#if 0
#asm
    xor ax, ax
    mov es, ax
    seg es
    mov[0], ax /* this SIGSEGVs quite nicely. :-) */
#endasm
#endif
    boot_kernel();
    
    while(1);
}
