#ifndef __ARCH_8086_KEYBOARD_H__
#define __ARCH_8086_KEYBOARD_H__

extern void keyboard_irq();

#define KBD_IO	0x60
#define KBD_CTL	0x61

#endif
