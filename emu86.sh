#!/bin/bash

# Run ELKS in EMU86 (basic 8086 emulator)
# EMU86 is part of the cross tools

# ELKS must be configured minimaly with 'cp config-emu86 .config'
# using the headless console and a ROM filesystem.

# First build ELKS with kernel and root FS in ROM
# Kernel image @ segment 0xE000 (top of 1024K address space)
# Root filesystem @ segment 0x8000 (assumes 512K RAM & 512K ROM)
# Skip the INT 19h bootstrap in the kernel image (+0x14)

exec emu86 -w 0xe0000 -f elks/arch/i86/boot/Image -w 0x80000 -f image/romfs.bin -x 0xe000:0x14 ${1+"$@"}
