#ifndef LX86_ARCH_SEGMENT_H
#define LX86_ARCH_SEGMENT_H

#include <linuxmt/types.h>

extern __u16 get_cs(void), get_ds(void), get_es(void), get_ss(void);
extern __u16 get_bp(void), setupw(unsigned short int, unsigned short int *);

extern pid_t get_pid(void);

/*@-namechecks@*/

extern short *_endtext, *_enddata, *_endbss;

/*@+namechecks@*/

#define put_user_char(val,ptr)	pokeb(current->t_regs.ds,ptr,val)
#define get_user_char(ptr)	peekb(current->t_regs.ds,ptr)

#define put_user(val,ptr)	pokew(current->t_regs.ds,ptr,val)
#define get_user(ptr)		peekw(current->t_regs.ds,ptr)

#define put_user_long(val,ptr)	poked(current->t_regs.ds,ptr,val)
#define get_user_long(ptr)	peekd(current->t_regs.ds,ptr)

#endif
