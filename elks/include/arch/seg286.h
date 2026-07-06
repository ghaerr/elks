#ifndef __ARCH_SEG286_H
#define __ARCH_SEG286_H

#include <linuxmt/types.h>

/*
 * 80286 protected-mode segment HAL for ELKS        (gated by CONFIG_286_PMODE)
 *
 * Design
 * ------
 * The kernel's segment abstraction is already the HAL boundary:
 *   - upper kernel (fs/, kernel/, net/) only ever holds an opaque seg_t and
 *     passes it to the far-memory primitives (peekb/peekw/pokeb/pokew,
 *     fmemcpyb/w, fmemsetb/w, fmemcmpb/w) and to seg_alloc()/segment_s.base.
 *   - those primitives do "mov es,seg ; ... es:[off]". That instruction works
 *     in both real mode (es = paragraph, phys = es*16+off) and protected mode
 *     (es = selector, phys = descriptor[es].base + off) -- the CPU resolves it
 *     per current mode. So the far-mem asm needs NO change.
 *
 * Therefore the 286-PM port is confined to three things, all below the HAL line:
 *   1. seg_t now carries a selector (not a paragraph). segment_s.base = selector;
 *      the physical base lives in the descriptor (desc_base() recovers it).
 *   2. seg_alloc()/seg_free() allocate physical paragraphs, then also
 *      allocate/free a GDT descriptor and return its selector in segment_s.base.
 *   3. boot sets up the GDT/IDT and enters protected mode; exec() loads
 *      selectors into CS/DS/SS instead of paragraphs.
 *
 * Real-mode build (CONFIG_286_PMODE off) is unaffected: seg_t stays a paragraph.
 * and none of the protected mode routines are linked into the kernel.
 */

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

#define GATE_INT286   0x86  /* present, DPL0, 286 interrupt gate (clears IF) */
#define GATE_TRAP286  0x87  /* present, DPL0, 286 trap gate (leaves IF)      */

/* lgdt/lidt operand: 16-bit limit + linear physical base (286 uses low 24 bits) */
struct dtr {
    word_t limit;
    addr_t base;
};

/* selector = (index << 3) | TI | RPL */
#define SEL_RPL0    0x00
#define SEL_RPL3    0x03
#define SEL_GDT     0x00
#define SEL_LDT     0x04
#define MK_SEL(index, ti, rpl)  (((unsigned)(index) << 3) | (ti) | (rpl))
#define SEL_INDEX(sel)          ((unsigned)(sel) >> 3)

/* ---- HAL hooks (implemented in pm286.c and pmode.S) ---- */

#define MAX_IDT_ENTRIES 129     /* 0..NR_IRQS required, needs 129 for 0x80 syscall */

void gdt_init(void);            /* build fixed GDT selector enries */
void pm_fault_panic(void);      /* PM exception handler: display info and halt */

/* load GDTR and IDTR, set PM via SMSW/LMSW, reload CS */
void enable_protected_mode(struct dtr *gdtr, struct dtr *idtr);

/* allocate a descriptor for [base, base+paras*16) with `access`; returns a
 * selector (0 on table-full). seg_alloc() calls this after reserving physram.
 */
sel_t  desc_alloc(addr_t base, segext_t paras, byte_t access);

/* rewrite / free an existing selector's descriptor */
void   desc_set(sel_t sel, addr_t base, segext_t paras, byte_t access);
void   desc_chaccess(sel_t sel, byte_t access);
void   desc_free(sel_t sel);

/* recover the physical base behind a selector (for DMA, exec relocation, etc.) */
addr_t desc_base(sel_t sel);

/* max valid byte offset in a selector's segment (its descriptor limit) */
segext_t desc_limit(sel_t sel); /* FIXME: will need 16M limit for 386 PM/fmemalloc */

/* install an interrupt gate (used by the IRQ/syscall path). */
void idt_gate_set(int vect, unsigned int offset, sel_t selector, byte_t access);

#endif /* __ARCH_SEG286_H */
