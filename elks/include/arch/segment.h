#ifndef LX86_ARCH_SEGMENT_H
#define LX86_ARCH_SEGMENT_H

#include <linuxmt/types.h>

#if 1

extern __u16 get_cs(void), get_ds(void), get_es(void), get_ss(void);

#else

#include <arch/asm.h>

#define get_cs()	asm("mov ax,cs")
#define get_ds()	asm("mov ax,ds")
#define get_es()	asm("mov ax,es")
#define get_ss()	asm("mov ax,ss")

#endif

extern __u16 setupw(unsigned short int);

#define setupb(p) ((char)setupw(p))

extern pid_t get_pid(void);

/*@-namechecks@*/

extern short *_endtext, *_enddata, *_endbss;

#endif
