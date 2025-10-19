#ifndef __LINUXMT_MEMORY_H
#define __LINUXMT_MEMORY_H

/* memory primitives */

#include <linuxmt/types.h>

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
#define _FP_SEG(fp)     ((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define _FP_OFF(fp)     ((unsigned)(unsigned long)(void __far *)(fp))
#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

/* unreal mode, A20 gate management */
void enable_unreal_mode(void);	/* requires 386+ CPU to call */
int enable_a20_gate(void);      /* returns 0 on fail */

/* XMS memory management */

/* possible values for xms_enabled and xms_bootopts */
#define XMS_DISABLED    0
#define XMS_UNREAL      1   /* using unreal mode and linear32_fmemcpy for block moves */
#define XMS_INT15       2   /* using BIOS INT 15 block move (or INT 1F on PC-98) */
#define XMS_LOADALL     3   /* using LOADALL 286 opcode and loadall_block_move */
extern int xms_bootopts;    /* xms=on or xms=int15 /bootopts setting, default off */

#ifdef CONFIG_FS_XMS
extern int xms_enabled;

typedef __u32 ramdesc_t;	/* special physical ram descriptor */
extern ramdesc_t df_cache_seg;  /* track cache segment or linear address if XMS */

/* allocate from XMS memory */
ramdesc_t xms_alloc(unsigned int kbytes);
extern unsigned int xms_alloc_ptr;  /* in kbyte increments */
#define KBYTES(n)   ((n) >> 10)

/* copy to/from XMS or far memory - XMS requires unreal mode and A20 gate enabled */
void xms_fmemcpyw(void *dst_off, ramdesc_t dst_seg, void *src_off, ramdesc_t src_seg,
		size_t count);
void xms_fmemcpyb(void *dst_off, ramdesc_t dst_seg, void *src_off, ramdesc_t src_seg,
		size_t count);
void xms_fmemset(void *dst_off, ramdesc_t dst_seg, size_t count);

/* low level copy - must have 386 CPU and xms_enabled before calling! */
void linear32_fmemcpyw(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
		size_t count);
void linear32_fmemcpyb(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
		size_t count);
void linear32_fmemset(void *dst_off, addr_t dst_seg, byte_t val, size_t count);

#else

typedef seg_t ramdesc_t;	/* ramdesc_t is just a regular segment descriptor */
#define xms_fmemcpyw	            fmemcpyw
#define xms_fmemcpyb	            fmemcpyb
#define xms_fmemset(off,seg,cnt)    fmemsetb(off, seg, 0, cnt)

#endif /* CONFIG_FS_XMS */

#endif
