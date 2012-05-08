#ifndef LX86_ARCH_KEYBOARD_H
#define LX86_ARCH_KEYBOARD_H

struct pt_regs;
extern void keyboard_irq(int,struct pt_regs *,void *);
extern void Console_set_vc(unsigned);

#ifdef CONFIG_ARCH_SIBO

extern int psiongetchar(void);

#endif

#define KBD_IO	0x60
#define KBD_CTL	0x61

#endif
