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

#define verify_area(mode, point, size) verfy_area(point, size)

extern int verfy_area(void *,size_t);
extern void memcpy_fromfs(void *,void *,size_t);
extern void memcpy_tofs(void *,void *,size_t);
extern void put_fs_long(unsigned long int,unsigned long int *);
extern void put_fs_byte(unsigned char,unsigned char *);
extern void put_fs_word(unsigned short int,unsigned short int *);
extern unsigned long get_fs_long(unsigned long int *);
extern unsigned char get_fs_byte(unsigned char *);
extern unsigned int get_fs_word(unsigned short int *);
extern int strlen_fromfs(char *);
extern int fs_memcmp(void *,void *,size_t);
extern int verified_memcpy_tofs(void *,void *,size_t);
extern int verified_memcpy_fromfs(void *,void *,size_t);

extern void mm_init(seg_t,seg_t);
extern seg_t mm_alloc(segext_t);
extern seg_t mm_realloc(seg_t);
extern seg_t mm_dup(seg_t);

extern void mm_free(seg_t);
extern int do_swapper_run(struct task_struct *);
extern unsigned int mm_get_usage(int,int);

extern void fmemcpy();

extern void pokeb();
extern void pokew();
extern void poked();

extern unsigned char peekb();
extern unsigned short int peekw();
extern unsigned long int peekd();

extern int mm_swapon();

#endif
#endif
