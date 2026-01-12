#!/usr/bin/env bash

# Run ELKS in EMU86 (basic 8086 emulator)
# EMU86 is part of the cross tools
# See https://github.com/ghaerr/emu86

# For ELKS ROM Configuration:
# ELKS must be configured minimaly with 'cp emu86-rom.config .config'
# This uses headless console, HLT on idle, ROM filesystem.

# First build ELKS with kernel and root FS in ROM
# Kernel image @ segment 0xE000 (top of 1024K address space as 64K BIOS extension)
# Root filesystem @ segment 0x8000 (assumes 512K RAM & 512K ROM)
# Skip the INT 19h bootstrap in the kernel image (+0x14)

ELKSDIR=.
EMU86DIR=../emu86

#
# run 'emu86.sh -i' to halt emulation at very first instruction (interactive mode)
# run 'emu86.sh -I image/fd2880.img' to add floppy image
#
exec ${EMU86DIR}/emu86 \
    -C 0xe062 -D 0x00c0 \
    -w 0xe0000 -f ${ELKSDIR}/elks/arch/i86/boot/Image \
    -w 0x80000 -f ${ELKSDIR}/image/romfs.bin \
    -S ${ELKSDIR}/elks/arch/i86/boot/system.sym \
    ${1+"$@"}
