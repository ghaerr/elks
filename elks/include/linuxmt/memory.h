#ifndef __LINUXMT_MEMORY_H
#define __LINUXMT_MEMORY_H

/* memory primitives */

#include <arch/types.h>

byte_t peekb (word_t off, seg_t seg);
word_t peekw (word_t off, seg_t seg);
long_t peekl (word_t off, seg_t seg);

void pokeb (word_t off, seg_t seg, byte_t val);
void pokew (word_t off, seg_t seg, word_t val);
void pokel (word_t off, seg_t seg, long_t val);

void fmemsetb (word_t off, seg_t seg, byte_t val, word_t count);
void fmemsetw (word_t off, seg_t seg, word_t val, word_t count);

void fmemcpyb (byte_t * dst_off, seg_t dst_seg, byte_t * src_off, seg_t src_seg, word_t count);
void fmemcpyw (byte_t * dst_off, seg_t dst_seg, byte_t * src_off, seg_t src_seg, word_t count);

word_t fmemcmpb (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count);
word_t fmemcmpw (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count);

/* macros for far pointers (a la Turbo C++ and Open Watcom) */
#define _FP_SEG(fp)	((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define _FP_OFF(fp)	((unsigned)(unsigned long)(void __far *)(fp))
#define _MK_FP(seg,off)	((void __far *)((((unsigned long)(seg)) << 16) | (off)))

#endif
