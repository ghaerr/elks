#ifndef __ARCH_8086_SYSTEM_H__
#define __ARCH_8086_SYSTEM_H__

void _FUNCTION(setup_arch, ());
void _FUNCTION(arch_setuptasks, ());
void _FUNCTION(save_regs, (__ptask task));
void _FUNCTION(load_regs, (__ptask task));

extern int arch_cpu;
#endif
