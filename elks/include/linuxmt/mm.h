#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>

extern unsigned long high_memory;

#ifdef __KERNEL__

#define VERIFY_READ 0
#define VERIFY_WRITE 1

#define verify_area(mode, point, size) verfy_area(point, size)

extern int verfy_area();
extern void memcpy_fromfs();
extern void memcpy_tofs();
extern void put_fs_long();
extern unsigned long get_fs_long();
extern void put_fs_byte();
extern unsigned char get_fs_byte();
extern void put_fs_word();
extern unsigned int get_fs_word();
extern int strlen_fromfs();

extern void mem_init();
extern void oom();
extern void si_meminfo();

#endif /* __KERNEL__ */

#endif
