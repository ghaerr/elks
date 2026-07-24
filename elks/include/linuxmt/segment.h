#ifndef __LINUXMT_SEGMENT_H
#define __LINUXMT_SEGMENT_H

/* macros for far pointers (a la Turbo C++ and Open Watcom) */
#define _FP_SEG(fp)     ((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define _FP_OFF(fp)     ((unsigned)(unsigned long)(void __far *)(fp))
#define _MK_FP(seg,off) ((void __far *) ((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))
#define _MK_LP(seg,off) ((unsigned long)((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

#endif
