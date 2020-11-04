#ifndef __LINUXMT_MM_H
#define __LINUXMT_MM_H

#include <linuxmt/types.h>
#include <linuxmt/list.h>

struct segment {
	list_s    all;
	list_s    free;
	seg_t     base;
	segext_t  size;
	word_t    flags;
	word_t    ref_count;
};

typedef struct segment segment_s;

// TODO: convert to tag
#define SEG_FLAG_FREE    0x0000
#define SEG_FLAG_USED	 0x0080
#define SEG_FLAG_ALIGN1K 0x0040
#define SEG_FLAG_TYPE	 0x000F
#define SEG_FLAG_CSEG	 0x0001
#define SEG_FLAG_DSEG	 0x0002
#define SEG_FLAG_EXTBUF	 0x0003
#define SEG_FLAG_RAMDSK	 0x0004

#ifdef __KERNEL__

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#include <linuxmt/memory.h>

#define verify_area(mode,point,size) verfy_area(point,size)

/*@-namechecks@*/

void memcpy_fromfs(void *,void *,size_t);
void memcpy_tofs(void *,void *,size_t);
int strlen_fromfs(void *,size_t);

/*@+namechecks@*/

int verfy_area(void *,size_t);
void put_user_char(unsigned char,void *);
void put_user(unsigned short,void *);
void put_user_long(unsigned long,void *);
unsigned char get_user_char(void *);
unsigned short get_user(void *);
unsigned long get_user_long(void *);
int verified_memcpy_tofs(void *,void *,size_t);
int verified_memcpy_fromfs(void *,void *,size_t);
int fs_memcmp(void *,void *,size_t);

/* Memory allocation */

segment_s * seg_alloc (segext_t, word_t);
void seg_free (segment_s *);

segment_s * seg_get (segment_s *);
void seg_put (segment_s *);

segment_s * seg_dup (segment_s *);

void mm_get_usage (unsigned int * free, unsigned int * used);

#endif // __KERNEL__
#endif // !__LINUXMT_MM_H
