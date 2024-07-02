#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H

#include <linuxmt/types.h>

extern seg_t kernel_cs, kernel_ds;
extern short *_endtext, *_endftext, *_enddata, *_endbss;
extern short endistack[];
extern unsigned int heapsize;

#endif
