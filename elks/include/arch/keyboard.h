#ifndef __ARCH_8086_KEYBOARD_H
#define __ARCH_8086_KEYBOARD_H

struct pt_regs;
extern void keyboard_irq(int,struct pt_regs *,void *);
extern void Console_set_vc(unsigned);

#ifdef CONFIG_ARCH_SIBO

extern int psiongetchar(void);

#endif

#endif
