#ifndef LX86_ARCH_KEYBOARD_H
#define LX86_ARCH_KEYBOARD_H

extern void keyboard_irq(int,struct pt_regs *,void *);

#define KBD_IO	0x60
#define KBD_CTL	0x61

#endif
