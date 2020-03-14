#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H

#include <linuxmt/types.h>

extern __u16 kernel_cs, kernel_ds;

// Get data from setup segment

extern word_t setupw (word_t);
extern byte_t setupb (word_t);

extern pid_t get_pid(void);

/*@-namechecks@*/

extern short *_endtext, *_enddata, *_endbss;

#endif /* !__ARCH_8086_SEGMENT_H */
