#!/bin/bash

# Run ELKS in EMU86 (basic 8086 emulator)
# EMU86 is part of the cross tools

# ELKS must be configured minimaly with 'config-emu86'
# with only core and using serial port for the console

# First run ELKS with kernel and root FS in ROM
# Kernel image @ segment 0xE000 (top of 1024K address space)
# Root filesystem @ segment 0x8000 (assumes 512K RAM & 512K ROM)
# Skip the INT 19h bootstrap in the kernel image (+0x32)

exec ../emu86/emu86 -w 0xe0000 -f elks/arch/i86/boot/Image -w 0x80000 -f image/romfs.bin -x 0xe000:0x32 ${1+"$@"}

# Give time to EMU86 to emulate the serial port
# and store the used PTY in 'emu86.pts'

sleep 1

# Get the used PTY and redirect serial port to console

pcat `cat emu86.pts`
