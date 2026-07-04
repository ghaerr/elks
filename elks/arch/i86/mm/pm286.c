/*
 * 80286 protected-mode descriptor allocator -- the seg_alloc() backend
 * for CONFIG_286_PMODE.  See <arch/seg286.h> for the overall design.
 *
 * Real-mode builds do not compile this file; seg_alloc() returns paragraphs.
 * In PM, seg_alloc() reserves physical paragraphs from the existing arena as
 * usual, then calls desc_alloc(phys_base, paras, DESC_*) and stores the
 * returned SELECTOR in segment_s.base.  The far-memory primitives then load
 * that selector into a segment register and the 286 resolves it via the GDT.
 */
#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/limits.h>
#include <arch/segment.h>
#include <arch/seg286.h>

/*
 * Config validation: protected mode cannot call the BIOS (real-mode IVT/
 * services are gone after PM entry), so any BIOS-backed driver selected in
 * .config would build fine and then fault at runtime with a raw EXC= dump.
 * Fail the compile instead, naming the required change.
 */
#if defined(CONFIG_BLK_DEV_BFD) || defined(CONFIG_BLK_DEV_BHD)
#error 286 PM: BIOS disk (BFD/BHD, int 0x13) cannot work - disable both, use CONFIG_BLK_DEV_FD (directfd) with root=df0
#endif
#ifdef CONFIG_CONSOLE_BIOS
#error 286 PM: BIOS console (int 0x10) cannot work - use CONFIG_CONSOLE_DIRECT or CONFIG_CONSOLE_SERIAL
#endif
#if defined(CONFIG_ARCH_IBMPC) && !defined(CONFIG_KEYBOARD_SCANCODE)
#error 286 PM: without CONFIG_KEYBOARD_SCANCODE the IBMPC build links kbd-poll/conio-bios (int 0x16) - enable it
#endif
#if defined(CONFIG_FS_XMS) || defined(CONFIG_FS_XMS_BUFFER) || defined(CONFIG_FS_XMS_RAMDISK)
#error 286 PM: XMS/unreal-mode paths are not ported - disable the CONFIG_FS_XMS* group
#endif

/* The Global Descriptor Table (4 KB, in the kernel data segment).
 * GDT_KCODE/KDATA are the kernel's own segments; GDT_FIRST_DYN..end are
 * handed out by desc_alloc() for process/buffer segments.
 */
static struct gdt_entry gdt[MAX_GDT_ENTRIES];

/* round-robin hint for the next candidate free slot */
static int next_dyn = GDT_FIRST_DYN;

void desc_set(sel_t sel, addr_t base, segext_t paras, byte_t access)
{
    struct gdt_entry *d = &gdt[SEL_INDEX(sel)];
    addr_t limit = PARA_BYTES(paras);

    if (limit == 0 || limit > 0x10000L)     /* a 286 segment is <= 64 KB */
        limit = 0x10000L;
    d->limit    = (word_t)(limit - 1);      /* limit is bytes-1 */
    d->base_lo  = (word_t)(base & 0xFFFF);
    d->base_hi  = (byte_t)((base >> 16) & 0xFF);
    d->access   = access;
    d->reserved = 0;                        /* must be 0 on a 286 */
}

/* change only the access byte (base/limit kept) -- exec flips its text seg
 * data(writable, to load) -> code(executable, to run) */
void desc_chaccess(sel_t sel, byte_t access)
{
    gdt[SEL_INDEX(sel)].access = access;
}

/* find a free GDT slot, fill it, return its selector (0 = table full) */
sel_t desc_alloc(addr_t base, segext_t paras, byte_t access)
{
    int i = next_dyn, scanned;

    for (scanned = 0; scanned < MAX_GDT_ENTRIES - GDT_FIRST_DYN; scanned++) {
        if (!(gdt[i].access & DESC_PRESENT)) {          /* P=0 => free */
            sel_t sel = MK_SEL(i, SEL_GDT, SEL_RPL0);
            desc_set(sel, base, paras, access);
            next_dyn = (i + 1 < MAX_GDT_ENTRIES) ? i + 1 : GDT_FIRST_DYN;
            return sel;
        }
        if (++i >= MAX_GDT_ENTRIES) i = GDT_FIRST_DYN;
    }
    return 0;                                            /* GDT full */
}

void desc_free(sel_t sel)
{
    gdt[SEL_INDEX(sel)].access = 0;                      /* clear Present */
}

addr_t desc_base(sel_t sel)
{
    struct gdt_entry *d = &gdt[SEL_INDEX(sel)];

    return ((addr_t)d->base_hi << 16) | d->base_lo;
}

segext_t desc_limit(sel_t sel)         /* max valid byte offset in the segment */
{
    return gdt[SEL_INDEX(sel)].limit;
}

/* ---- Interrupt Descriptor Table (2 KB, in the kernel data segment) ---- */
static struct idt_gate idt[256];

void idt_gate_set(unsigned int vect, word_t offset, sel_t selector, byte_t access)
{
    struct idt_gate *g = &idt[vect];

    g->offset   = offset;
    g->selector = selector;
    g->zero     = 0;
    g->access   = access;
    g->reserved = 0;
}

/* Point every vector at the fault-catch stub (in kernel code = SEL_KCODE) so an
 * exception between PM entry and irq_init() is reported, not a triple fault.
 * irq_init() later overwrites the live IRQ/syscall/INT0/INT2 vectors (phase 5b). */
void idt_init(void)
{
    unsigned int v;
    for (v = 0; v < 32; v++)    /* per-vector stubs so the reporter knows the vector */
        idt_gate_set(v, (word_t)(pm_flt_base + v*8), SEL_KCODE, GATE_INT286);
    for (v = 32; v < 256; v++)
        idt_gate_set(v, (word_t)pm_flt_other, SEL_KCODE, GATE_INT286);
}

/* called once from crt0.S just before start_kernel: fill the kernel's own
 * descriptors, build the GDTR/IDTR, then switch to protected mode.  Does not
 * return to real mode. */
void gdt_init(void)
{
    static struct dtr gdtr, idtr;
    addr_t   code_base  = (addr_t)kernel_cs << 4;
    addr_t   data_base  = (addr_t)kernel_ds << 4;
    segext_t text_par   = BYTES_PARA((unsigned)_endtext);
    /* fartext is loaded immediately after text (non-HMA boot). */
    addr_t   ftext_base = code_base + ((addr_t)text_par << 4);

    /* phase 4b: separate descriptors for near text, far text and data.
     * setup.S has patched the kernel's far-call/far-data segments to these
     * selectors (SEL_KCODE/SEL_KFTEXT/SEL_KDATA) instead of paragraphs. */
    desc_set(MK_SEL(GDT_NULL,   SEL_GDT, SEL_RPL0), 0,          0,                     0);
    desc_set(MK_SEL(GDT_KCODE,  SEL_GDT, SEL_RPL0), code_base,  text_par,              DESC_KCODE);
    desc_set(MK_SEL(GDT_KDATA,  SEL_GDT, SEL_RPL0), data_base,  0x1000,                DESC_KDATA);
    desc_set(MK_SEL(GDT_KFTEXT, SEL_GDT, SEL_RPL0), ftext_base, BYTES_PARA((unsigned)_endftext), DESC_KCODE);
    /* same base/limit as KDATA but executable+readable: IRQ trampolines are built
     * in the kernel data segment, and an IDT gate needs an executable selector. */
    desc_set(MK_SEL(GDT_KDATA_EXEC, SEL_GDT, SEL_RPL0), data_base, 0x1000, DESC_KCODE);
    /* boot setup data lives at the SEG_SETUP_DATA paragraph; setupb/setupw read it via
     * SEL_SETUP in PM (a raw paragraph can't be loaded into a segment register). */
    desc_set(MK_SEL(GDT_SETUP, SEL_GDT, SEL_RPL0), (addr_t)SEG_SETUP_DATA << 4, BYTES_PARA(512), DESC_KDATA);
    /* boot options (/bootopts) area, copied once into the kernel during init */
    desc_set(MK_SEL(GDT_OPTSEG, SEL_GDT, SEL_RPL0), (addr_t)DEF_OPTSEG << 4, 0x40, DESC_KDATA);
    /* BIOS data area (real-mode seg 0x40, physical 0x400, 256 bytes) -- read by drivers */
    desc_set(MK_SEL(GDT_BIOSDATA, SEL_GDT, SEL_RPL0), (addr_t)SEG_BIOSDATA << 4, BYTES_PARA(256), DESC_KDATA);
    /* text video memory (real-mode seg 0xb800, physical 0xB8000) -- direct console */
    desc_set(MK_SEL(GDT_VIDEO, SEL_GDT, SEL_RPL0), (addr_t)SEG_VIDEO << 4, 0x1000, DESC_KDATA);
    /* directfd DMA/track buffer; base=SEG_TRACK<<4 (physical paragraph), far-mem copies in PM */
    desc_set(MK_SEL(GDT_TRACK, SEL_GDT, SEL_RPL0), (addr_t)SEG_TRACK << 4, 0x200, DESC_KDATA);

    idt_init();                 /* every vector -> fault-catch stub (until irq_init) */

    gdtr.limit = (word_t)(sizeof(gdt) - 1);
    gdtr.base  = data_base + (addr_t)(unsigned)&gdt;        /* linear addr of GDT */
    idtr.limit = (word_t)(sizeof(idt) - 1);
    idtr.base  = data_base + (addr_t)(unsigned)&idt;        /* linear addr of IDT */

    pm_switch(&gdtr, &idtr);    /* enter PM; returns here in protected mode */
}
