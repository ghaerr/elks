/*
 * 80286/80386+ Protected-mode descriptor allocator.
 *
 * In PM, seg_alloc() reserves physical paragraphs from the existing arena as
 * usual, then calls desc_alloc to allocate a selector, which is stored instead
 * of the real mode segment in segment_s.base.  The far-memory primitives then load
 * that selector into a segment register and the CPU resolves it via the GDT.
 */
#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/memory.h>
#include <linuxmt/limits.h>
#include <arch/segment.h>
#include <arch/seg286.h>
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

/* Global Descriptor Table */
static struct gdt_entry gdt[MAX_GDT_ENTRIES];

/* round-robin hint for the next candidate free slot */
static int next_dyn = GDT_FIRST_DYN;

/* set GDT entry for passed selector */
sel_t desc_set(sel_t sel, addr_t base, addr_t limit, byte_t access)
{
    struct gdt_entry *d = &gdt[SEL_INDEX(sel)];

    /* Allow selector limit of max 64K for now for automatic 80286 compatibility.
     * User mode fmemalloc allocations are also limited to 64K max segments
     * unless 32-bit instructions are used within the application.
     */
    if ((limit >> 16) > 1) {                /* limit size to 64K max for now */
        printk("desc_set: %08lx limit > 64K\n", limit);
        return 0;
    }

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
    gdt[SEL_INDEX(sel)].access = access;
}

/* find a free GDT slot, fill it, return its selector (0 = table full or limit > 64K) */
sel_t desc_alloc(addr_t base, addr_t limit, byte_t access)
{
    int i, scanned;
    sel_t sel = 0;
    flag_t flags;

    save_flags(flags);  /* may be called at interrupt time through xms_fmemcpy */
    clr_irq();
    i = next_dyn;
    for (scanned = 0; scanned < MAX_GDT_ENTRIES - GDT_FIRST_DYN; scanned++) {
        if (gdt[i].access == 0) {
            sel = desc_set(MK_SEL(i, SEL_GDT, SEL_RPL0), base, limit, access);
            next_dyn = (i + 1 < MAX_GDT_ENTRIES) ? i + 1 : GDT_FIRST_DYN;
            break;
        }
        if (++i >= MAX_GDT_ENTRIES) i = GDT_FIRST_DYN;
    }
    restore_flags(flags);
    return sel;
}

void desc_free(sel_t sel)
{
    gdt[SEL_INDEX(sel)].access = 0;                      /* clear Present */
}

/* return selector physical base address (< 16M) */
addr_t desc_base(sel_t sel)
{
    struct gdt_entry *d = &gdt[SEL_INDEX(sel)];

    return ((addr_t)d->base_hi << 16) | d->base_lo;
}

/* return selector limit (< 64K). FIXME: will need 16M limit for 386 PM/fmemalloc */
addr_t desc_limit(sel_t sel)
{
    return gdt[SEL_INDEX(sel)].limit_lo;
}


/* Set an entry in the IDT; uses sel_idt which initially points to real mode IVT at 0:0.
 * After idt_init() sel_idt is updated to SEL_IDT for PM access to the IDT.
 */
static seg_t sel_idt = 0;   /* init for real mode segment 0 */
void idt_gate_set(unsigned int vect, unsigned int offset, sel_t selector, byte_t access)
{
    struct idt_gate __far *g;

    if (vect >= MAX_IDT_ENTRIES) {
        printk("idt_gate_set: invalid vector %d\n", vect);
        return;
    }
    g = _MK_FP(sel_idt, vect * sizeof(struct idt_gate));
    g->offset   = offset;
    g->selector = selector;
    g->zero     = 0;
    g->access   = access;
    g->reserved = 0;
}

/* Point IDT vectors at the fault-catch handler so an exception between
 * PM entry and irq_init() is reported, not a triple fault.
 * irq_init() later overwrites the live IRQ/syscall/INT0/INT2 vectors.
 */
static void idt_init(void)
{
    int v;

    for (v = 0; v < 32; v++)    /* per-vector entries so the reporter knows the vector */
        idt_gate_set(v, (unsigned)pm_fault_panic + v*8, SEL_KCODE, GATE_INT286);
    for (v = 32; v < MAX_IDT_ENTRIES; v++)
        idt_gate_set(v, (unsigned)pm_fault_panic + 32*8, SEL_KCODE, GATE_INT286);
}

/* Called from near text start_kernel. Build the GDT/IDT, then switch to PM.
 * No return to real mode, and BIOS calls aren't allowed after the switch.
 */
void gdt_init(void)
{
    addr_t   data_base  = (addr_t)kernel_ds << 4;
    static struct dtr gdtr, idtr;

    /* enable A20 while still in real mode (calls BIOS) */
    if (!enable_a20_gate())
        printk("A20 fail ");

    /* separate descriptors for near text, far text and data.
     * setup.S has patched the kernel's far-call/far-data segments to these
     * selectors (SEL_KCODE/SEL_KFTEXT/SEL_KDATA) instead of paragraphs.
     */
    desc_set(MK_SEL(GDT_NULL,   SEL_GDT, SEL_RPL0), 0,          0,         0);

    desc_set(MK_SEL(GDT_KCODE,  SEL_GDT, SEL_RPL0),
        (addr_t)kernel_cs << 4, (unsigned)_endtext, DESC_KCODE);

    desc_set(MK_SEL(GDT_KDATA,  SEL_GDT, SEL_RPL0), data_base, 65536L, DESC_KDATA);

    if (kernel_ftext) {
        desc_set(MK_SEL(GDT_KFTEXT, SEL_GDT, SEL_RPL0),
            (addr_t)kernel_ftext << 4, (unsigned)_endftext, DESC_KCODE);
    }

    /* KCODE same base/limit as KDATA but executable+readable. IRQ trampolines are
     * built in the kernel data segment, and an IDT gate needs an executable selector.
     */
    desc_set(MK_SEL(GDT_KDATA_EXEC, SEL_GDT, SEL_RPL0), data_base, 65536L, DESC_KCODE);

    /* IDT replaces real mode IVT interrupt vector table + 8 bytes from 0:0 to 0:0407.
     * NOTE: With the normal MAX_IDT_ENTRIES of 129 (required for syscall INT 0x80),
     * the IDT which uses 8 bytes/entry will overwrite the first 1K IVT plus the
     * first 8 bytes of the BIOS data area which starts at 0x40:0. The first 8
     * bytes of the BDA contain the port addresses of COM1-COM4, which aren't used
     * by ELKS setupb/setupw anyways.
     */
    desc_set(MK_SEL(GDT_IDT, SEL_GDT, SEL_RPL0), 0, MAX_IDT_ENTRIES * 8, DESC_KDATA);

    /* setupb/setupw setup.S data segment */
    desc_set(MK_SEL(GDT_SETUP, SEL_GDT, SEL_RPL0), SEG_INITSEG << 4, 512, DESC_KDATA);

    /* boot options (/bootopts) segment */
    desc_set(MK_SEL(GDT_OPTSEG, SEL_GDT, SEL_RPL0), SEG_OPTSEG << 4, 1024, DESC_KDATA);

    /* BIOS data area */
    desc_set(MK_SEL(GDT_BIOSDATA, SEL_GDT, SEL_RPL0), SEG_BIOSDATA << 4, BYTES_PARA(256),
        DESC_KDATA);

    /* text video memory */
    desc_set(MK_SEL(GDT_VIDEO, SEL_GDT, SEL_RPL0), (addr_t)SEG_VIDEO << 4, 32768L,
        DESC_KDATA);

    /* direct floppy DMA track buffer */
    desc_set(MK_SEL(GDT_TRACKBUF, SEL_GDT, SEL_RPL0), SEG_TRACK << 4, TRACKSEGSZ,
        DESC_KDATA);

    /* ATA/CF DMA sector buffer */
    desc_set(MK_SEL(GDT_DMABUF, SEL_GDT, SEL_RPL0), SEG_DMASEG << 4, 512, DESC_KDATA);

    /* initialize IVT using real mode sel_idt segment 0 to fault-catch stubs */
    idt_init();

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = data_base + (unsigned)&gdt; /* linear address of GDT */
    idtr.limit = MAX_IDT_ENTRIES * sizeof(struct idt_gate) - 1;
    idtr.base  = 0;                         /* IDT replaces real mode IVT at 0:0 */
    sel_idt = SEL_IDT;                      /* use SEL_IDT for idt_get_set from now on */

    enable_protected_mode(&gdtr, &idtr);    /* enter PM; returns here in protected mode */
}
