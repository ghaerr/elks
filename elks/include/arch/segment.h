#ifndef LX86_ARCH_SEGMENT_H
#define LX86_ARCH_SEGMENT_H

extern int get_cs(void), get_ds(void), get_es(void), get_ss(void);

#define DS	current->t_regs.ds

extern short *_endtext, *_enddata, *_endbss;

#define put_user_char(val,ptr)	pokeb(DS,ptr,val)
#define get_user_char(ptr)	peekb(DS,ptr)

#define put_user(val,ptr)	pokew(DS,ptr,val)
#define get_user(ptr)		peekw(DS,ptr)

#define put_user_long(val,ptr)	poked(DS,ptr,val)
#define get_user_long(ptr)	peekd(DS,ptr)

#undef DS

#endif
