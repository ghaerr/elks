#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H


extern short *_endtext,*_enddata,*_endbss;
extern int get_ss(),get_ds(),get_es(),get_ss(),get_cs();

#define put_user(val,ptr) pokew(current->t_regs.ds,ptr,val)
#define get_user(ptr) peekw(current->t_regs.ds,ptr)

#if 0
#define put_user(x,ptr) __put_user((unsigned long)(x),(ptr),sizeof(*(ptr)))
#define get_user(ptr) ((__typeof__(*(ptr)))__get_user((ptr),sizeof(*(ptr))))
#endif

#endif /* __ARCH_8086_SEGMENT_H */
