#ifndef LX86_ARCH_SEGMENT_H
#define LX86_ARCH_SEGMENT_H

extern int get_cs(void), get_ds(void), get_es(void), get_ss(void);

extern short *_endtext, *_enddata, *_endbss;

#define put_user_char(val,ptr)	pokeb(current->t_regs.ds,ptr,val)
#define get_user_char(ptr)	peekb(current->t_regs.ds,ptr)

#define put_user(val,ptr)	pokew(current->t_regs.ds,ptr,val)
#define get_user(ptr)		peekw(current->t_regs.ds,ptr)

#define put_user_long(val,ptr)	poked(current->t_regs.ds,ptr,val)
#define get_user_long(ptr)	peekd(current->t_regs.ds,ptr)

#undef DS

#endif
