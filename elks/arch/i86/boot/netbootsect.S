
!
! SYS_SIZE is the number of clicks (16 bytes) to be loaded.
! 0x7F00 is 0x7F000 bytes = 508kB, more than enough for current
! versions of linux which compress the kernel
!
#include <linuxmt/config.h>
SYSSIZE = DEF_SYSSIZE
!
!	bootsect.s		Copyright (C) 1991, 1992 Linus Torvalds
!	modified by Drew Eckhardt
!	modified by Bruce Evans (bde)
! 	modified by Alan Cox for Linux/ELKS and 8088 compatiblity
!	modified for Linux/16 by Chad Page
!
! bootsect.s is loaded at 0x7c00 by the bios-startup routines, and moves
! itself out of the way to address 0x90000, and jumps there.
!
! bde - should not jump blindly, there may be systems with only 512K low
! memory.  Use int 0x12 to get the top of memory, etc.
!
! It then loads 'setup' directly after itself (0x90200), and the system
! at 0x10000, using BIOS interrupts.
!
! NOTE! currently system is at most (8*65536-4096) bytes long. This should
! be no problem, even in the future. I want to keep it simple. This 508 kB
! kernel size should be enough, especially as this doesn't contain the
! buffer cache as in minix (and especially now that the kernel is
! compressed :-)
!
! The loader has been made as simple as possible, and continuous
! read errors will result in a unbreakable loop. Reboot by hand. It
! loads pretty fast by getting whole tracks at a time whenever possible.

.text

SETUPSECS = 4				! nr of setup-sectors
BOOTSEG   = 0x07C0			! original address of boot-sector
INITSEG   = DEF_INITSEG			! we move boot here - out of the way

SYSSEG    = DEF_SYSSEG			! system loaded at 0x10000 (65536).
SYSSEGB   = DEF_SYSSEG + 2

! ROOT_DEV is now written by "build".
ROOT_DEV = 0
#ifndef SVGA_MODE
#define SVGA_MODE VGA_ASK
#endif
#ifndef RAMDISK
#define RAMDISK 0
#endif

#include <linuxmt/boot.h>

! ld86 requires an entry symbol. This may as well be the usual one.
.globl	_main
_main:
.org 0x0000
	.long 0x1B031336
	.long 0x00000004
	.word 0x0000
	.word INITSEG
	.word 0x0200
	.word INITSEG

	.long 0x00000004
	.long ((INITSEG << 4) + 0x0200)
	.long (SETUPSECS * 512)
	.long (SETUPSECS * 512)

#ifdef CONFIG_PRELOAD_RAMDISK
	.long 0x00000004
#else
	.long 0x04000004      /* bit 26 is end marker ! */
#endif
	.long (SYSSEG << 4)
	.long (SYSSIZE << 4)
	.long (SYSSIZE << 4)

#ifdef CONFIG_PRELOAD_RAMDISK
	.long 0x04000004
	.long 0x00060000
	.long 0x00020000
	.long 0x00020000
#endif

.org 0x01EF
	.word 0
setup_sects:
	.byte 0
root_flags:
	.word ROOTFLAGS
syssize:
	.word SYSSIZE
unused:
	.word 0
ram_size:
	.word RAMDISK
vid_mode:
	.word SVGA_MODE
root_dev:
	.word ROOT_DEV
boot_flag:
	.word 0xAA55
.org 0x200
