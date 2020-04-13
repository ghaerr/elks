#ifndef __LINUXMT_MEMORY_H
#define __LINUXMT_MEMORY_H

/* memory primitives */

#include <arch/types.h>

extern byte_t peekb (word_t off, seg_t seg);
extern word_t peekw (word_t off, seg_t seg);

extern void pokeb (word_t off, seg_t seg, byte_t val);
extern void pokew (word_t off, seg_t seg, word_t val);

extern void fmemsetb (word_t off, seg_t seg, byte_t val, word_t count);
extern void fmemsetw (word_t off, seg_t seg, word_t val, word_t count);

extern void fmemcpyb (byte_t * dst_ptr, seg_t dst_seg, byte_t * src_off, seg_t src_seg, word_t count);
extern void fmemcpyw (byte_t * dst_off, seg_t dst_seg, byte_t * src_off, seg_t src_seg, word_t count);

extern word_t fmemcmpb (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count);
extern word_t fmemcmpw (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count);

#endif
