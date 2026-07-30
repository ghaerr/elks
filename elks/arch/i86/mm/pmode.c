/*
 * 80286/80386+ Protected mode implementation for ELKS
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/segment.h>
#include <linuxmt/memory.h>
#include <linuxmt/limits.h>
#include <linuxmt/init.h>

#include <arch/segment.h>
#include <arch/seg286.h>
#include <arch/system.h>
#include <arch/irq.h>

/* Disallow certain configurations for protected mode builds */
#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD)
#error BIOS INT 13h fd/hd disallowed in PM build. Use CONFIG_BLK_DEV_FD with root=df0
#endif
#ifdef CONFIG_CONSOLE_BIOS
#error BIOS INT 10h console disallowed in PM build. Use CONFIG_CONSOLE_DIRECT or CONFIG_CONSOLE_SERIAL
#endif
#if defined(CONFIG_ARCH_IBMPC) && !defined(CONFIG_KEYBOARD_SCANCODE)
#error BIOS INT 16h non-scancode keyboard disallowed in PM build. Use CONFIG_KEYBOARD_SCANCODE
#endif

/* segment/selectors for accessing GDT and IDT during and after init */
static sel_t sel_gdt;           /* GDT segment then selector */
static sel_t sel_idt = 0;       /* IDT segment/selector, init to real mode segment 0 */

/* round-robin hint for the next candidate free slot */
static sel_t next_dyn = SEL_FIRST_DYN;

/* set GDT entry for passed selector */
sel_t desc_set(sel_t sel, addr_t base, addr_t limit, byte_t access)
{
    struct gdt_entry __far *d = _MK_FP(sel_gdt, SEL_INDEX(sel));

    d->limit_lo     = (word_t)limit - 1;    /* limit is bytes-1 */
    d->base_lo      = base & 0xFFFF;
    d->base_hi      = (base >> 16) & 0xFF;
    d->access       = access;
    d->fl_limit_hi  = 0;                    /* must be 0 on a 286 */
    d->base_24      = 0;                    /* must be 0 on a 286 */
    return sel;
}

/* Change access byte for passed selector */
void desc_chaccess(sel_t sel, byte_t access)
{
    struct gdt_entry __far *d = _MK_FP(sel_gdt, SEL_INDEX(sel));
    d->access = access;
}

/* Find a free GDT slot, fill it, return its selector (0 = table full or limit > 64K).
 *
 * In PM, seg_alloc() reserves physical paragraphs from the existing arena as
 * usual, then calls desc_alloc to allocate a selector, which is stored instead
 * of the real mode segment in segment_s.base.  The far-memory primitives then load
 * that selector into a segment register and the CPU resolves it via the GDT.
 */
sel_t desc_alloc(addr_t base, seloff_t limit, byte_t access)
{
    struct gdt_entry __far *d;
    sel_t i, sel = 0;
    int scanned;
    flag_t flags;

    save_flags(flags);  /* may be called at interrupt time through xms_fmemcpy */
    clr_irq();
    i = next_dyn;
    for (scanned = 0; scanned < MAX_GDT_ENTRIES - (SEL_FIRST_DYN >> 3); scanned++) {
        d = _MK_FP(sel_gdt, i);
        if (d->access == 0) {
            sel = desc_set(i, base, limit, access);
            next_dyn = (i + 8 < GDT_SIZE) ? i + 8 : SEL_FIRST_DYN;
            break;
        }
        if ((i += 8) >= GDT_SIZE) i = SEL_FIRST_DYN;
    }
    restore_flags(flags);
    if (!sel) printk("desc_alloc: FAIL!\n");
    return sel;
}

void desc_free(sel_t sel)
{
    struct gdt_entry __far *d = _MK_FP(sel_gdt, SEL_INDEX(sel));
    d->access = 0;                  /* clear present bit */
}

/* return selector physical base address (< 16M) */
addr_t desc_base(sel_t sel)
{
    struct gdt_entry __far *d = _MK_FP(sel_gdt, SEL_INDEX(sel));

    return ((addr_t)d->base_hi << 16) | d->base_lo;
}

/* return selector limit (< 64K). FIXME: will need 16M limit for 386 PM/fmemalloc */
seloff_t desc_limit(sel_t sel)
{
    struct gdt_entry __far *d = _MK_FP(sel_gdt, SEL_INDEX(sel));
    return d->limit_lo;
}


/* Set an entry in the IDT; uses sel_idt which initially points to real mode IVT at 0:0.
 * After idt_init() sel_idt is updated to SEL_IDT for PM access to the IDT.
 */
void idt_gate_set(unsigned int vect, unsigned int proc, sel_t selector, byte_t access)
{
    struct idt_gate __far *g;

    if (vect >= MAX_IDT_ENTRIES) {
        printk("idt_gate_set: invalid vector %d\n", vect);
        return;
    }
    g = _MK_FP(sel_idt, vect * sizeof(struct idt_gate));
    g->offset   = proc;
    g->selector = selector;
    g->wcount   = 0;            /* word count (=0 for 286 task and interrupt gates) */
    g->access   = access;
    g->offset_hi= 0;            /* high 16 bits handler address (=0 for 286) */
}

/* Point IDT vectors at pm_fault_vector to display exception and panic
 * on non-handled faults, rather than a triple fault and CPU reset.
 */
static void idt_init(void)
{
    int v;

    for (v = 0; v < 32; v++)    /* per-vector entries so the reporter knows the vector */
        idt_gate_set(v, (unsigned)pm_fault_vector + v*8, SEL_KCODE, GATE_INT286);
    for (v = 32; v < MAX_IDT_ENTRIES; v++)
        idt_gate_set(v, (unsigned)pm_fault_vector + 32*8, SEL_KCODE, GATE_INT286);
}

/* Called after pm_early_init after kernel CS, DS and .fartext set */
void INITPROC pm_init(void)
{
    desc_set(SEL_SETUP, SEG_INITSEG << 4, 512, DESC_KDATA);     /* setup.S data */
    desc_set(SEL_OPTSEG, SEG_OPTSEG << 4, 1024, DESC_KDATA);    /* /bootopts segment */
    desc_set(SEL_BIOSDATA, SEG_BIOSDATA << 4, 256, DESC_KDATA); /* BIOS data area */
    desc_set(SEL_VIDEO, (addr_t)SEG_VIDEO << 4, 32768L, DESC_KDATA); /* text video area */

#ifdef TRACKSEGSZ
    /* low-memory DMA track buffer (direct floppy) */
    desc_set(SEL_TRACKBUF, SEG_TRACK << 4, TRACKSEGSZ, DESC_KDATA);
#endif

#ifdef DMASEGSZ
    /* shared low-memory DMA bounce buffer (ATA/CF) */
    desc_set(SEL_DMABUF, SEG_DMASEG << 4, DMASEGSZ, DESC_KDATA);
#endif
}

/* Setup CS/DS/.fartext selectors, as setup.S already patched the kernel's
 * CS/DS/far-call segments to fixed selectors, then switch to PM.
 * No return to real mode, and BIOS calls aren't allowed after the switch.
 */
void pm_early_init(void)
{
    addr_t   data_base  = (addr_t)kernel_ds << 4;
    struct dtr gdtr, idtr;

    /* enable A20 while still in real mode (calls BIOS) */
    if (!enable_a20_gate())
        printk("A20 fail ");

    /* Init sel_gdt now with a segment address to allow desc_set to build the
     * initial GDT in real mode, which is placed just after the kernel data segment.
     * Then create the SEL_GDT, SEL_IDT ad other selectors, which will be used after
     * entering PM.
     */
    sel_gdt = kernel_ds + 0x1000;
    fmemsetw(0, sel_gdt, 0, GDT_SIZE >> 1);

    //desc_set(SEL_NULL, 0, 0, 0);
    desc_set(SEL_KCODE, (addr_t)kernel_cs << 4, (unsigned)_endtext, DESC_KCODE);
    desc_set(SEL_KDATA, data_base, 65536, DESC_KDATA);
    if (kernel_ftext)
        desc_set(SEL_KFTEXT, (addr_t)kernel_ftext << 4, (unsigned)_endftext, DESC_KCODE);

    /* KCODE same base/limit as KDATA but executable+readable. IRQ trampolines are
     * built in the kernel data segment, and an IDT gate needs an executable selector.
     */
    desc_set(SEL_KDATA_EXEC, data_base, 65536, DESC_KCODE);

    /* IDT replaces real mode IVT interrupt vector table + 8 bytes from 0:0 to 0:0407.
     * NOTE: With the normal MAX_IDT_ENTRIES of 129 (required for syscall INT 0x80),
     * the IDT which uses 8 bytes/entry will overwrite the first 1K IVT plus the
     * first 8 bytes of the BIOS data area which starts at 0x40:0. The first 8
     * bytes of the BDA contain the port addresses of COM1-COM4, which aren't used
     * by ELKS setupb/setupw anyways.
     */
    desc_set(SEL_IDT, 0, MAX_IDT_ENTRIES * 8, DESC_KDATA);
    desc_set(SEL_GDT, (addr_t)sel_gdt << 4, GDT_SIZE, DESC_KDATA);

    /* initialize IVT using real mode sel_idt segment 0 to fault-catch stubs */
    idt_init();

    gdtr.limit = GDT_SIZE - 1;
    gdtr.base  = (addr_t)sel_gdt << 4;      /* linear address of GDT */
    idtr.limit = MAX_IDT_ENTRIES * sizeof(struct idt_gate) - 1;
    idtr.base  = 0;                         /* IDT replaces real mode IVT at 0:0 */
    sel_idt = SEL_IDT;                      /* use SEL_IDT in idt_get_set from now on */
    sel_gdt = SEL_GDT;                      /* and SEL_GDT in desc_alloc */

    enable_protected_mode(&gdtr, &idtr);    /* enter PM; returns here in protected mode */
}

/* unhandled protected mode fault - display info and panic */
void pm_exception_handler(int arg)
{
    unsigned int *p = (unsigned int *)&arg;

    panic("unhandled PM fault\n"
        "DS %04x ES %04x EXC %04x CODE %04x CS %04x IP %04x FLAG %04x STK %04x %04x",
        p[0], p[1], p[2], p[3], p[5], p[4], p[6], p[7], p[8]);
}
