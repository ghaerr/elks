#ifndef __ARCH_SEG286_H
#define __ARCH_SEG286_H
/*
 * 80286 protected-mode segment HAL for ELKS        (gated by CONFIG_286_PMODE)
 *
 * Design
 * ------
 * The kernel's segment abstraction is ALREADY the HAL boundary:
 *   - upper kernel (fs/, kernel/, net/) only ever holds an opaque seg_t and
 *     passes it to the far-memory primitives (peekb/peekw/pokeb/pokew,
 *     fmemcpyb/w, fmemsetb/w, fmemcmpb/w) and to seg_alloc()/segment_s.base.
 *   - those primitives do "mov es,seg ; ... es:[off]". That instruction works
 *     in BOTH real mode (es = paragraph, phys = es*16+off) AND protected mode
 *     (es = selector, phys = descriptor[es].base + off) -- the CPU resolves it
 *     per current mode. So the far-mem asm needs NO change.
 *
 * Therefore the 286-PM port is confined to three things, all below the HAL line:
 *   1. seg_t now carries a SELECTOR (not a paragraph). segment_s.base = selector;
 *      the physical base lives in the descriptor (desc_base() recovers it).
 *   2. seg_alloc()/seg_free() allocate physical paragraphs as today, then also
 *      allocate/free a GDT (or per-process LDT) descriptor and return its
 *      selector in segment_s.base.  (arch/i86/mm/pm286.c)
 *   3. boot sets up the GDT/IDT and enters protected mode; exec() loads
 *      selectors into CS/DS/SS instead of paragraphs (it already goes through
 *      seg_alloc + segment_s.base, so the change is small).
 *
 * Real-mode build (CONFIG_286_PMODE off) is unaffected: seg_t stays a paragraph
 * and none of this header's code is compiled.
 */

#include <linuxmt/types.h>

/* ---- 80286 segment descriptor: 8 bytes; high word MUST be 0 on a 286 ----
 * (on a 386 the reserved word holds limit[16:19]+flags+base[24:31]; writing 0
 *  there yields 286-equivalent semantics, so this layout is forward-compatible) */
struct gdt_entry {
    word_t  limit;          /* segment limit in bytes-1  (0..0xFFFF => <=64K)   */
    word_t  base_lo;        /* physical base bits 0..15                          */
    byte_t  base_hi;        /* physical base bits 16..23 (24-bit base => 16 MB)  */
    byte_t  access;         /* P | DPL | S | type | A  (see DESC_* below)        */
    word_t  reserved;       /* 0 on 286                                          */
};
typedef struct gdt_entry gdt_entry_t;

/* access byte: bit7=Present 6-5=DPL 4=S(1=code/data) 3=E 2=C/ED 1=R/W 0=Accessed */
#define DESC_PRESENT 0x80   /* P bit (set in every DESC_* below); 0 => free slot */
#define DESC_KCODE  0x9A    /* present, DPL0, code, readable        */
#define DESC_KDATA  0x92    /* present, DPL0, data, writable        */
#define DESC_UCODE  0xFA    /* present, DPL3, code, readable        */
#define DESC_UDATA  0xF2    /* present, DPL3, data, writable        */

/* ---- 80286 interrupt/trap gate: 8 bytes (high word 0 on a 286) ---- */
struct idt_gate {
    word_t  offset;         /* handler offset within `selector`             */
    word_t  selector;       /* code (or data-as-code) selector of handler   */
    byte_t  zero;           /* always 0                                     */
    byte_t  access;         /* P | DPL | gate type (GATE_* below)           */
    word_t  reserved;       /* 0 on a 286                                   */
};
typedef struct idt_gate idt_gate_t;
#define GATE_INT286   0x86  /* present, DPL0, 286 interrupt gate (clears IF) */
#define GATE_TRAP286  0x87  /* present, DPL0, 286 trap gate (leaves IF)      */

/* selector = (index << 3) | TI | RPL */
#define SEL_RPL0    0x00
#define SEL_RPL3    0x03
#define SEL_GDT     0x00
#define SEL_LDT     0x04
#define MK_SEL(index, ti, rpl)  (((unsigned)(index) << 3) | (ti) | (rpl))
#define SEL_INDEX(sel)          ((unsigned)(sel) >> 3)

/* fixed GDT layout (kernel-private selectors) */
#define GDT_NULL        0   /* required null descriptor          */
#define GDT_KCODE       1   /* kernel CS  (near .text)           */
#define GDT_KDATA       2   /* kernel DS = SS                    */
#define GDT_KFTEXT      3   /* kernel far text (.fartext, medium model) */
#define GDT_KDATA_EXEC  4   /* kernel data aliased as readable CODE (IRQ trampolines) */
#define GDT_SETUP       5   /* boot setup-data area (REL_INITSEG, read by setupb/setupw) */
#define GDT_OPTSEG      6   /* boot options area (DEF_OPTSEG, /bootopts read once at init) */
#define GDT_BIOSDATA    7   /* BIOS data area (segment 0x40, base 0x400) */
#define GDT_VIDEO       8   /* CGA/EGA/VGA text video memory (0xB8000) */
#define GDT_TRACK       9   /* directfd DMA/track buffer (TRACKSEG) for far-mem copies in PM */
#define GDT_FIRST_DYN   10  /* first dynamically-allocated selector */

/* fixed kernel selectors (index<<3, GDT, RPL0) -- must match setup.S patches */
#define SEL_KCODE       0x08
#define SEL_KDATA       0x10
#define SEL_KFTEXT      0x18
#define SEL_SETUP       0x28    /* GDT[5]: base = REL_INITSEG<<4, for setupb/setupw in PM */
#define SEL_OPTSEG      0x30    /* GDT[6]: base = DEF_OPTSEG<<4, for the /bootopts copy in PM */
#define SEL_BIOSDATA    0x38    /* GDT[7]: base = 0x400, BIOS data area (seg 0x40) reads in PM */
#define SEL_VIDEO       0x40    /* GDT[8]: base = 0xB8000, text video memory (VideoSeg) in PM */
#define SEL_TRACK       0x48    /* GDT[9]: base = TRACKSEG<<4, directfd DMA/track buffer in PM */
#define SEL_KDATA_EXEC  0x20    /* GDT[4]: same base as KDATA, but executable+readable;
                                 * IDT gates point here so the CPU can run the trampolines
                                 * that int_handler_add() builds in the kernel data segment */
#define GDT_ENTRIES     512 /* 512 * 8 = 4 KB GDT                 */

/* paragraph (16-byte) helpers shared with the real-mode arena allocator */
#define PARA_BYTES(paras)   ((addr_t)(paras) << 4)
#define BYTES_PARA(bytes)   (((bytes) + 15) >> 4)

#ifdef CONFIG_286_PMODE

/* ---- HAL hooks (implemented in arch/i86/mm/pm286.c and arch/i86/boot) ---- */

/* boot: build GDT[GDT_KCODE/KDATA], load GDTR+IDTR, set CR0.PE, far-jump to PM */
void gdt_init(void);

/* allocate a descriptor for [base, base+paras*16) with `access`; returns a
 * selector (0 on table-full). seg_alloc() calls this after reserving physram. */
unsigned int desc_alloc(addr_t base, segext_t paras, byte_t access);

/* rewrite / free an existing selector's descriptor */
void   desc_set(unsigned int sel, addr_t base, segext_t paras, byte_t access);
void   desc_chaccess(unsigned int sel, byte_t access);
void   desc_free(unsigned int sel);

/* recover the physical base behind a selector (for DMA, exec relocation, etc.) */
addr_t desc_base(unsigned int sel);

/* max valid byte offset in a selector's segment (its descriptor limit) */
word_t desc_limit(unsigned int sel);

/* IDT: zero the table + point every vector at the fault-catch stub (called from
 * gdt_init before entering PM); install one gate (used by the IRQ/syscall path). */
void idt_init(void);
void idt_gate_set(unsigned int vect, word_t offset, word_t selector, byte_t access);

#endif /* CONFIG_286_PMODE */

#endif /* __ARCH_SEG286_H */
