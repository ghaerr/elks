/*
 * arch/i86/286pmode/main.c
 *
 * main source file for the 286pmode ELKS kernel extender
 */

#include <arch/irq.h>

#include "descriptor.h"
#include "tss.h"

/* #define DEBUG_2ND_MONITOR */
/* #define ELKS_AT_RING_1 */

#define PMODE_CODESEL 0x08
#define PMODE_DATASEL 0x10
#define PMODE_SCRNSEL 0x18
#define PMODE_TEMPSEL 0x20

#define PMODE_TSSSEL 0x28
#define PMODE_LDTSEL 0x30

#ifdef ELKS_AT_RING_1
#  define ELKS_CODESEL 0x39
#  define ELKS_DATASEL 0x41
#  define SETUP_DATASEL 0x49
#else
#  define ELKS_CODESEL 0x38
#  define ELKS_DATASEL 0x40
#  define SETUP_DATASEL 0x48
#endif

#define GATE_INTERRUPT 0x86
#define GATE_TRAP      0x87

extern unsigned short _endtext;
extern unsigned short _enddata;
extern unsigned short _endbss;
extern unsigned short _elksheader;
extern unsigned char arch_cpu;

extern int bootstack;		/* this really wants to be extern void, but bcc sucks. */

unsigned short realmode_ds;	/* used for calculating the address of the IDT */

/* 0x20 processor exceptions, 0x10 hardware ints */
struct descriptor idt[0x30];

#define IDT_LIMIT 0x17f

struct descriptor initial_gdt[10] = {
    {0x0000, 0x0000, 0x00, 0x00, 0x0000},	/* NULL segment */
    {0xffff, 0x0020, 0x01, 0x9b, 0x0000},	/* ring 0 code segment (ours) */
    {0xffff, 0x0020, 0x01, 0x93, 0x0000},	/* ring 0 data segment (ours) */
    {0xffff, 0x0000, 0x00, 0x93, 0x0000},	/* ring 0 screen data segment */
    {0xffff, 0x0000, 0x00, 0x93, 0x0000},	/* ring 0 temporary data segment */
    {0x002b, 0x0000, 0x00, 0x81, 0x0000},	/* initial TSS */
    {0x0000, 0x0000, 0x00, 0x82, 0x0000},	/* 0-length LDT for TSS */
#ifdef ELKS_AT_RING_1
    {0x0000, 0x0000, 0x00, 0xbb, 0x0000},	/* ring 1 code segment (ELKSs) */
    {0xffff, 0x0000, 0x00, 0xb3, 0x0000},	/* ring 1 data segment (ELKSs) */
    {0xffff, 0x1000, 0x00, 0xb3, 0x0000},	/* ring 1 data segment (setup) */
#else
    {0x0000, 0x0000, 0x00, 0x9b, 0x0000},	/* ring 0 code segment (ELKSs) */
    {0xffff, 0x0000, 0x00, 0x93, 0x0000},	/* ring 0 data segment (ELKSs) */
    {0xffff, 0x1000, 0x00, 0x93, 0x0000},	/* ring 0 data segment (setup) */
#endif
};

#define GDT_LIMIT 0x4f

/* most of these fields can be nil without affecting anything. */
struct tss initial_tss = {
    0x0000,				/* backlink */
    0x0000, PMODE_DATASEL,		/* cpl0 sp, cpl0 ss */
    0x0000, 0x0000,			/* cpl1 sp, cpl1 ss */
    0x0000, 0x0000,			/* cpl2 sp, cpl2 ss */
    0x0000, 0x0000,			/* ip, flags */
    0x0000, 0x0000, 0x0000, 0x0000,	/* ax, cx, dx, bx */
    0x0000, 0x0000, 0x0000, 0x0000,	/* sp, bp, si, di */
    0x0000, 0x0000, 0x0000, 0x0000,	/* es, cs, ss, ds */
    PMODE_LDTSEL,			/* ldt */
};

char hexchars[0x10] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,
};

/* these all want to be extern void, but you saw my previous comment. */
extern int exc_00, exc_01, exc_02, exc_03, exc_04, exc_05, exc_06, exc_07;
extern int exc_08, exc_09, exc_0a, exc_0b, exc_0c, exc_0d, exc_0e, exc_10;
extern int exc_11, exc_xx;

extern int irq_0, irq_1, irq_2, irq_3, irq_4, irq_5, irq_6, irq_7;
extern int irq_8, irq_9, irq_a, irq_b, irq_c, irq_d, irq_e, irq_f;

int *irqhandlers[0x30] = {
    &exc_00, &exc_01, &exc_02, &exc_03, &exc_04, &exc_05, &exc_06, &exc_07,
    &exc_08, &exc_09, &exc_0a, &exc_0b, &exc_0c, &exc_0d, &exc_0e, &exc_xx,
    &exc_10, &exc_11, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx,
    &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx, &exc_xx,
    &irq_0, &irq_1, &irq_2, &irq_3, &irq_4, &irq_5, &irq_6, &irq_7,
    &irq_8, &irq_9, &irq_a, &irq_b, &irq_c, &irq_d, &irq_e, &irq_f,
};

unsigned short get_ds(void)
{
    asm("mov ax,ds");
}

unsigned short get_flags(void)
{
    asm("pushf");
    asm("pop ax");
}

void set_flags(unsigned short flags)
{
#ifndef S_SPLINT_S
#asm
    pop bx
    pop ax
    push ax
    push bx
    push ax
    popf
#endasm
#endif
}

void load_gdtr(void)
{
#ifndef S_SPLINT_S
#asm
#ifdef DEBUG_2ND_MONITOR
    push es
    mov ax, #0xb000
    mov es, ax
    mov ax, #0x0736
    seg es
    mov [0x24], ax
    pop es
#endif
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
    lgdt [bp - 6]
#ifdef DEBUG_2ND_MONITOR
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
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov al, #0x20
    stosw
    mov dx,[bp - 2]
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov dx,[bp - 4]

    mov bl, dh
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov al, #0x20
    stosw
    mov dx, #_initial_gdt
    mov bl, dh
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    pop di
    pop es
#if 0
    db 0xeb, 0xfe
#endif
#endif
#endif
    add sp, #6
    pop bp
#ifdef DEBUG_2ND_MONITOR
    push es
    mov ax, #0xb000
    mov es, ax
    mov ax, #0x0734
    seg es
    mov [0x28], ax
    pop es
#endif
#endasm
#endif
}

void load_idtr(void)
{
#ifndef S_SPLINT_S
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
    lidt [bp - 6]
#ifdef DEBUG_2ND_MONITOR
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
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov al, #0x20
    stosw
    mov dx,[bp - 2]
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov dx,[bp - 4]
    mov bl, dh
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov al, #0x20
    stosw
    mov dx, #_idt
    mov bl, dh
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    pop di
    pop es
#if 0
    db 0xeb, 0xfe
#endif
#endif
#endif
    add sp, #6
    pop bp
#endasm
#endif
}

void load_tr(void)
{
#ifndef S_SPLINT_S
#asm
    mov ax, #PMODE_TSSSEL
    ltr ax
#endasm
#endif
}

void set_idt_entry(unsigned short interrupt, unsigned short cs,
                   unsigned short ip, unsigned short type)
{
    unsigned short flags = get_flags();

    clr_irq();

    idt[interrupt].limit = ip;
    idt[interrupt].baseaddr0 = cs;
    idt[interrupt].flags = (unsigned char) type;
    idt[interrupt].baseaddr1 = 0;
    idt[interrupt].reserved = 0;

    set_flags(flags);
}

void idt_init(void)
{
    unsigned short int i;

    for (i = 0; i < 0x30; i++)
	set_idt_entry(i, PMODE_CODESEL, (unsigned short int) irqhandlers[i],
			 GATE_INTERRUPT);
}

char irq_mesg[] = "received irq ";

void do_irq(irqnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax)
     unsigned short irqnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax;
{
#ifndef S_SPLINT_S
#asm
    push bp
    mov bp, sp
#ifdef DEBUG_2ND_MONITOR
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
    mov dx,[bp + 4]
    mov bl, dh
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    pop es
#endif
    dw 0xfeeb

#if 0

foo:
    jmp foo

#endif

    pop bp
#endasm
#endif
}

char exc_mesg[] = "recieved exception ";

void do_exc(excnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax)
     unsigned short excnum, es, ds, di, si, bp, ignore, bx, dx, cx, ax;
{
#ifndef S_SPLINT_S
#asm
    push bp
    mov bp, sp
#ifdef DEBUG_2ND_MONITOR
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
    mov dx,[bp + 4]
    mov bl, dh
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dh
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    shr bl, #4
    mov al,[bx + _hexchars]
    stosw
    mov bl, dl
    and bl, #0x0f
    mov al,[bx + _hexchars]
    stosw
    pop es
#endif
    db 0xeb, 0xfe
    pop bp
#endasm
#endif
}

void revector_8259s(unsigned short where)
{
#ifndef S_SPLINT_S
#asm
    pop dx	/* cheap trick to get the parameter into bx */
    pop bx
    push bx
    push dx
    mov al, #0x11
    out 0x20, al
    db 0xeb, 0x00

    /* this code may not require quite so much delay for I/O, */
    db 0xeb, 0x00	/* any opinions? */
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
#endif
}

void disable_irqs(void)
{
#ifndef S_SPLINT_S
#asm
    mov al, #0xff
    out 0x21, al
    out 0xa1, al
#endasm
#endif
}

void fix_flags_for_pmode(void)
{
    if (arch_cpu > 6) {
#ifndef S_SPLINT_S
#asm
	pushfd
	pop eax
	and eax, #0x00000fff
	push eax
	popfd
#endasm
#endif
    } else {
#ifndef S_SPLINT_S
#asm
	pushf
	pop ax
	and ax, #0x0fff
	push ax
	popf
#endasm
#endif
    }
}

void switch_to_pmode(void)
{
#ifndef S_SPLINT_S
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
#endif
}

/*
 * FIXME: I don't trust this compiler to generate accurate code for this.
 * either prove that it doesn't, or change it.
 *
 * FIXME: bcc doesn't generate anything like efficient code for this.
 * I'm not sure what the reason is, but the code looks amazingly repetitive.
 */
void init_ksegs(unsigned short textlen)
{
    unsigned long int eh = ((unsigned long int) _elksheader + 2);

    initial_gdt[7].baseaddr0 = (unsigned short) (eh << 4);
    initial_gdt[7].baseaddr1 = (unsigned short) ((eh >> 0xc) & 0xff);
    initial_gdt[7].limit = textlen;

    eh += textlen;
    initial_gdt[8].baseaddr0 = (unsigned short) (eh << 4);
    initial_gdt[8].baseaddr1 = (unsigned short) ((eh >> 0xc) & 0xff);
}

void bogus_magic(void)
{
#ifdef DEBUG_2ND_MONITOR
#ifndef S_SPLINT_S
#asm
    mov ax, #PMODE_SCRNSEL
    mov es, ax
    mov ax, #0x0736
    mov di, #0x580
    mov cx, #42
    repz stosw
    dw 0xfeeb
#endasm
#endif
#endif
}

void boot_kernel(void)
{
    initial_gdt[4].baseaddr0 = (unsigned short) (_elksheader << 4);
    initial_gdt[4].baseaddr1 = (unsigned short) ((_elksheader >> 0xc) & 0xff);

#ifndef S_SPLINT_S
#asm
    mov ax, #PMODE_TEMPSEL
    mov es, ax
    seg es
    cmp[0], #0x0301
    jz boot_kernel_good_sig
    call _bogus_magic

boot_kernel_good_sig:
    seg es
    mov dx,[8]
    mov ax, ds
    mov es, ax
    push dx
    call _init_ksegs
    pop dx
    mov ax, #PMODE_TEMPSEL
    mov ds, ax
    mov bx,[8]
    mov si,[12]
    mov dx,[16]
    mov ax, #ELKS_DATASEL
    mov ds, ax
    mov es, ax

#if 1				/* this can be left in even if ELKS is running at ring 0. */

    push #ELKS_DATASEL
    push #0xfffe

#endif

    push #ELKS_CODESEL
    push #0x0003

    retf

#endasm
#endif
}

void pmodekern_init(void)
{
    realmode_ds = get_ds();

    initial_gdt[1].limit = _endtext;
    initial_gdt[2].limit = _enddata + _endbss;
    initial_gdt[2].baseaddr0 += _endtext;
    load_gdtr();

    clr_irq();
    switch_to_pmode();

    initial_gdt[3].baseaddr0 = 0x0000;
    initial_gdt[3].baseaddr1 = 0x0b;
    initial_gdt[3].limit = 0xffff;

    revector_8259s(0x2820);

    idt_init();

    load_idtr();

    disable_irqs();

    set_irq();

#if 1

    initial_tss.cpl0_sp = (unsigned short) &bootstack;
    initial_gdt[4].baseaddr0 = initial_gdt[2].baseaddr0;
    initial_gdt[4].baseaddr1 = initial_gdt[2].baseaddr1;

    /* FIXME: should carry into baseaddr1 */

    initial_gdt[4].baseaddr0 += (unsigned short) &initial_tss;

    load_tr();

#endif

#if 0
#ifndef S_SPLINT_S
#asm
    xor ax, ax
    mov es, ax
    seg es
    mov[0], ax		/* this SIGSEGVs quite nicely. :-) */
#endasm
#endif
#endif

    boot_kernel();

    while (1)
	/* Do nothing */;
}
