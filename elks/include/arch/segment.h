#ifndef __ARCH_8086_SEGMENT_H
#define __ARCH_8086_SEGMENT_H

extern short *_endtext,*_enddata,*_endbss;
extern int get_ss(),get_ds(),get_es(),get_ss(),get_cs();

#define put_user_char(val,ptr) pokeb(current->t_regs.ds,ptr,val)
#define get_user_char(ptr) peekb(current->t_regs.ds,ptr)

#define put_user(val,ptr) pokew(current->t_regs.ds,ptr,val)
#define get_user(ptr) peekw(current->t_regs.ds,ptr)

#define put_user_long(val,ptr) poked(current->t_regs.ds,ptr,val)
#define get_user_long(ptr) peekd(current->t_regs.ds,ptr)

#endif /* __ARCH_8086_SEGMENT_H */
