#!/usr/bin/env bash

# Run ELKS in EMU86 (basic IA16 emulator)
# EMU86 is part of the cross tools
# See https://github.com/mfld-fr/emu86

# For ELKS ROM Configuration:
# ELKS must be configured minimally with 'cp emu86-rom.config .config'
# This uses headless console, HLT on idle, ROM filesystem.

# Kernel image @ segment 0xE000 (as 64K BIOS extension)
# Root filesystem @ segment 0x8000 (assumes 512K RAM & 512K ROM)

exec emu86 -w 0xe0000 -f elks/arch/i86/boot/Image -w 0x80000 -f image/romfs.bin ${1+"$@"}

# For ELKS Full ROM Configuration:
# ELKS must be configured minimally with 'cp emu86-rom-full.config .config'
# This adds MINIX/FAT filesytems with BIOS driver

exec emu86 -w 0xe0000 -f elks/arch/i86/boot/Image -w 0x80000 -f image/romfs.bin -I image/fd1440.bin ${1+"$@"}

# For ELKS disk image Configuration:
# ELKS must be configured with 'cp emu86-disk.config .config'
# This uses headless console, HLT on idle, no CONFIG_IDE_PROBE

exec emu86 -I image/fd1440.bin ${1+"$@"}
