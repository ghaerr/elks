#ifndef __LINUXMT_MM_H
#define __LINUXMT_MM_H

#include <linuxmt/types.h>
#include <linuxmt/list.h>

struct segment {
	list_s    node;
	seg_t     base;
	segext_t  size;
	word_t    flags;
	word_t    ref_count;
};

typedef struct segment segment_s;

// TODO: convert to tag
#define SEG_FLAG_USED	0x0080
#define SEG_FLAG_TYPE	0x000F
#define SEG_FLAG_CSEG	0x0001
#define SEG_FLAG_DSEG	0x0002
#define SEG_FLAG_EXTBUF	0x0003

#ifdef __KERNEL__

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#define MM_MEM	0

#include <linuxmt/memory.h>

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

void mm_init (seg_t, seg_t);

segment_s * seg_alloc (segext_t);
void seg_free (segment_s *);

segment_s * seg_get (segment_s *);
void seg_put (segment_s *);

segment_s * seg_dup (segment_s *);

void mm_stat (seg_t, seg_t);
unsigned int mm_get_usage (int, int);

#endif
#endif
