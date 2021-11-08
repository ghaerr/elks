#ifndef __LINUXMT_MEMORY_H
#define __LINUXMT_MEMORY_H

/* memory primitives */

#include <arch/types.h>
#include <stddef.h>

byte_t peekb (word_t off, seg_t seg);
word_t peekw (word_t off, seg_t seg);
long_t peekl (word_t off, seg_t seg);

byte_t setupb (word_t);		/* Get data from setup segment */
word_t setupw (word_t);

void pokeb (word_t off, seg_t seg, byte_t val);
void pokew (word_t off, seg_t seg, word_t val);
void pokel (word_t off, seg_t seg, long_t val);

void fmemsetb (void * off, seg_t seg, byte_t val, size_t count);
void fmemsetw (void * off, seg_t seg, word_t val, size_t count);

void fmemcpyb (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg, size_t count);
void fmemcpyw (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg, size_t count);

word_t fmemcmpb (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg, size_t count);
word_t fmemcmpw (void * dst_off, seg_t dst_seg, void * src_off, seg_t src_seg, size_t count);

/* macros for far pointers (a la Turbo C++ and Open Watcom) */
#define _FP_SEG(fp)	((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define _FP_OFF(fp)	((unsigned)(unsigned long)(void __far *)(fp))
#define _MK_FP(seg,off)	((void __far *)((((unsigned long)(seg)) << 16) | (off)))

int enable_unreal_mode(void);
int enable_a20_gate(void);

#endif
