#ifndef __LINUXMT_MM_H
#define __LINUXMT_MM_H

#include <linuxmt/types.h>
#include <linuxmt/list.h>

struct segment {
	list_s    all;
	list_s    free;
	seg_t     base;
	segext_t  size;
	byte_t    flags;
	byte_t    ref_count;
	word_t    pid;
};

typedef struct segment segment_s;

// TODO: convert to tag
#define SEG_FLAG_FREE    0x00
#define SEG_FLAG_USED	 0x80
#define SEG_FLAG_ALIGN1K 0x40
#define SEG_FLAG_TYPE	 0x0F
#define SEG_FLAG_CSEG	 0x01   /* app code segment */
#define SEG_FLAG_DSEG	 0x02   /* app auto (stack/heap) data segment */
#define SEG_FLAG_DDAT	 0x03   /* app data segment */
#define SEG_FLAG_FDAT	 0x04   /* app fmemalloc far data */
#define SEG_FLAG_EXTBUF	 0x05   /* ext/main memory buffers */
#define SEG_FLAG_RAMDSK	 0x06   /* ram disk buffers */

#ifdef __KERNEL__

#include <linuxmt/kernel.h>

void memcpy_fromfs(void *,void *,size_t);
void memcpy_tofs(void *,void *,size_t);
int strlen_fromfs(void *,size_t);

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#define verify_area(mode,point,size) verfy_area(point,size)

int verfy_area(void *,size_t);
void put_user_char(unsigned char,void *);
void put_user(unsigned short,void *);
void put_user_long(unsigned long,void *);
unsigned char get_user_char(const void *);
unsigned short get_user(void *);
unsigned long get_user_long(void *);
int verified_memcpy_tofs(void *,void *,size_t);
int verified_memcpy_fromfs(void *,void *,size_t);
int fs_memcmp(const void *,const void *,size_t);

/* Memory allocation */

segment_s * seg_alloc (segext_t, word_t);
void seg_free (segment_s *);

segment_s * seg_get (segment_s *);
void seg_put (segment_s *);
segment_s * seg_dup (segment_s *);

void seg_free_pid(pid_t pid);

void mm_get_usage (unsigned int * free, unsigned int * used);

#endif /* __KERNEL__ */

#endif
