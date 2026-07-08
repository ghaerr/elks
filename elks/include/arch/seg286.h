#ifndef __ARCH_SEG286_H
#define __ARCH_SEG286_H

#include <linuxmt/types.h>

/*
 * 80286/80386+ Protected-mode segment HAL for ELKS
 *
 * Design
 * ------
 * The kernel's segment abstraction is already the HAL boundary:
 *   - upper kernel (fs/, kernel/, net/) only ever holds an opaque seg_t and
 *     passes it to the far-memory primitives (peekb/peekw/pokeb/pokew,
 *     fmemcpyb/w, fmemsetb/w, fmemcmpb/w) and to seg_alloc/segment_s.base.
 *   - those primitives do "mov es,seg ; ... es:[off]". That instruction works
 *     in both real mode (es = paragraph, phys = es*16+off) and protected mode
 *     (es = selector, phys = descriptor[es].base + off) -- the CPU resolves it
 *     per current mode. So the far-mem asm needs no change.
 *
 * Therefore the PM port is confined to three things, all below the HAL line:
 *   1. seg_t now carries a selector (not a segment). segment_s.base = selector;
 *      the physical base lives in the descriptor.
 *   2. seg_alloc()/seg_free() allocate physical paragraphs, then also
 *      allocate/free a GDT descriptor and return its selector in segment_s.base.
 *   3. boot sets up the GDT/IDT and enters protected mode; exec() loads
 *      selectors into CS/DS/SS instead of segments.
 *
 * Real-mode build is unaffected: seg_t stays a paragraph, and none of the
 * protected mode routines are linked into the kernel.
 */

/* 80286/80386+ segment descriptor */
struct gdt_entry {
    word_t  limit_lo;       /* segment limit bits 15:0 in bytes - 1             */
    word_t  base_lo;        /* physical base bits 15:0                          */
    byte_t  base_hi;        /* physical base bits 23:16 (24-bit base = 16 MB)   */
    byte_t  access;         /* P | DPL | S | type | A  (see DESC_* below)       */
    byte_t  fl_limit_hi;    /* flags[7:4], limit[3:0] bits 19:16 (0 on 286)     */
    byte_t  base_24;        /* physical base bits 31:24 (0 on 286)              */
};

/* access byte: bit7=Present 6-5=DPL 4=S(1=code/data) 3=E 2=C/ED 1=R/W 0=Accessed */
#define DESC_PRESENT 0x80   /* P bit (set in every DESC_* below); 0 => free slot */
#define DESC_KCODE  0x9A    /* present, DPL0, code, readable        */
#define DESC_KDATA  0x92    /* present, DPL0, data, writable        */
#define DESC_UCODE  0xFA    /* present, DPL3, code, readable        */
#define DESC_UDATA  0xF2    /* present, DPL3, data, writable        */

/* 80286/80386+ interrupt/trap gate */
struct idt_gate {
    word_t  offset;         /* handler address bits 15:0                        */
    word_t  selector;       /* code (or data-as-code) selector of handler       */
    byte_t  wcount;         /* word count (=0 for 80286 interrupt/trap gates)   */
    byte_t  access;         /* P | DPL | gate type (GATE_* below)               */
    word_t  offset_hi;      /* handler address bits 23:16 (=0 for 80286)        */
};

#define GATE_INT286   0x86  /* present, DPL0, 286 interrupt gate (clears IF) */
#define GATE_TRAP286  0x87  /* present, DPL0, 286 trap gate (leaves IF)      */

/* descriptor table register operand for LGDT and LIDT instructions */
struct dtr {
    word_t limit;           /* size of table - 1 */
    addr_t base;            /* 24-bit linear physical base address from real mode */
};

/* selector = (index << 3) | TI | RPL */
#define SEL_RPL0    0x00
#define SEL_RPL3    0x03
#define SEL_GDT     0x00
#define SEL_LDT     0x04
#define MK_SEL(index, ti, rpl)  (((unsigned)(index) << 3) | (ti) | (rpl))
#define SEL_INDEX(sel)          ((unsigned)(sel) >> 3)

/* ---- HAL hooks (implemented in pmode.c and pmode.S) ---- */

/* The ELKS PM Interupt Descriptor Table uses the same physical memory as
 * the real mode Interrupt Vector Table, except that the IDT uses 8 bytes/entry
 * whereas the IVT uses just 4. Thus, only the first 128 entries will fit into
 * the original IVT locations before extending into the BIOS Data Area starting
 * at physical address 0x400.
 * NOTE: MAX_IDT_ENTRIES > 128 uses 8 bytes of the BDA per additional entry!
 */
#define MAX_IDT_ENTRIES 129     /* 0..NR_IRQS required, needs 129 for 0x80 syscall */

void gdt_init(void);            /* build fixed GDT selector enries */
void pm_fault_vector(void);     /* PM exception handler: display info and halt */
void pm_exception_handler(int arg); /* display exception info and panic */

/* load GDTR and IDTR, set PM via SMSW/LMSW, reload CS */
void enable_protected_mode(struct dtr *gdtr, struct dtr *idtr);

/* allocate a descriptor for [base, base+paras*16) with `access`; returns a
 * selector (0 on table-full). seg_alloc() calls this after reserving physram.
 */
sel_t  desc_alloc(addr_t base, addr_t limit, byte_t access);

/* rewrite / free an existing selector's descriptor */
sel_t  desc_set(sel_t sel, addr_t base, addr_t limit, byte_t access);
void   desc_chaccess(sel_t sel, byte_t access);
void   desc_free(sel_t sel);

/* recover the physical base behind a selector (for DMA, exec relocation, etc.) */
addr_t desc_base(sel_t sel);

/* max valid byte offset in a selector's segment (its descriptor limit) */
addr_t desc_limit(sel_t sel);

/* install an interrupt gate (used by the IRQ/syscall path). */
void idt_gate_set(unsigned int vect, unsigned int proc, sel_t selector, byte_t access);

#endif /* __ARCH_SEG286_H */
