#ifndef LX86_ARCH_SEGMENT_H
#define LX86_ARCH_SEGMENT_H

#include <linuxmt/types.h>

extern __u16 kernel_cs, kernel_ds;

extern __u16 setupw(unsigned short int);

#define setupb(p) ((char)setupw(p))

extern pid_t get_pid(void);

/*@-namechecks@*/

extern short *_endtext, *_enddata, *_endbss;

#endif
