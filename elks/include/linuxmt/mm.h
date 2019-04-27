#ifndef LX86_LINUXMT_MM_H
#define LX86_LINUXMT_MM_H

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>

extern unsigned long high_memory;

#ifdef __KERNEL__

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#define MM_MEM	0
#ifdef CONFIG_SWAP
#define MM_SWAP	1
#endif

/* memory primitives */
/* TODO: move these to a new library header memory.h */

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


#define verify_area(mode,point,size) verfy_area(point,size)

/*@-namechecks@*/

extern void memcpy_fromfs(void *,void *,size_t);
extern void memcpy_tofs(void *,void *,size_t);

extern int strnlen_fromfs(void *,size_t);

/*@+namechecks@*/

extern int verfy_area(void *,size_t);
extern void put_user_long(unsigned long int,void *);
extern void put_user_char(unsigned char,void *);
extern void put_user(unsigned short int,void *);
extern unsigned long int get_user_long(void *);
extern unsigned char get_user_char(void *);
extern unsigned short int get_user(void *);
extern int fs_memcmp(void *,void *,size_t);
extern int verified_memcpy_tofs(void *,void *,size_t);
extern int verified_memcpy_fromfs(void *,void *,size_t);

/* Memory allocation */

extern void mm_init(seg_t,seg_t);
extern seg_t mm_alloc(segext_t);
extern seg_t mm_realloc(seg_t,segext_t);
extern void mm_free(seg_t);
extern void mm_get(seg_t);
extern void mm_put(seg_t);
extern seg_t mm_dup(seg_t);
extern void mm_stat(seg_t,seg_t);
extern unsigned int mm_get_usage(int,int);

extern int do_swapper_run(struct task_struct *);

/* FIXME: Following function prototypes are apparently phantom ones,
 *	  so are defined here with void parameters for now.
 */

extern int mm_swapon(void);

#endif
#endif
