/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED

/*
 * Just accept the defaults unless you know what you are doing
 */

/*
 * Advanced - for Developers and Hackers only
 */
#undef  CONFIG_EXPERIMENTAL
#undef  CONFIG_OBSOLETE
#undef  CONFIG_NOT_YET

/*
 * Architecture
 */
#define CONFIG_ARCH_AUTO 1
#undef  CONFIG_ARCH_IBMPC
#undef  CONFIG_ARCH_SIBO
#define CONFIG_PC_AUTO 1
#undef  CONFIG_PC_XT
#undef  CONFIG_PC_AT
#undef  CONFIG_PC_MCA
#undef  CONFIG_ROMCODE
#undef  CONFIG_ROMCODE

/*
 * Model-specific features
 */
#define CONFIG_IBMPC_CLONE 1
#undef  CONFIG_IBMPC_COMPAQ
#undef  CONFIG_COMPAQ_FAST

/*
 * Hardware Facilities
 */
#define CONFIG_CPU_8086 1
#undef  CONFIG_CPU_80186
#undef  CONFIG_CPU_80286
#undef  CONFIG_CPU_80386
#undef  CONFIG_CPU_80486

/*
 * System Hardware
 */
#undef  CONFIG_HW_NO_FPU
#undef  CONFIG_HW_NO_PS2_MOUSE
#undef  CONFIG_HW_NO_VGA

/*
 * Block Devices
 */
#undef  CONFIG_HW_NO_FLOPPY_DRIVE
#undef  CONFIG_HW_NO_HARD_DRIVE

/*
 * Character Devices
 */

/*
 * BIOS Facilities
 */
#undef  CONFIG_HW_NO_KEYBOARD_BIOS
#undef  CONFIG_MEM_EXTENDED_MEMORY
#define CONFIG_MEM_EXTENDED_MEMORY_SIZE (0)
#undef  CONFIG_HW_VIDEO_HOC
#define CONFIG_HW_VIDEO_LINES_PER_CHARACTER (8)
#define CONFIG_HW_VIDEO_LINES_PER_SCREEN (25)
#undef  CONFIG_HW_259_USE_ORIGINAL_MASK
#define CONFIG_BOGOMIPS (0)
#undef  CONFIG_HW_USE_INT13_FOR_FLOPPY
#undef  CONFIG_HW_NO_SEEK_FOR_FLOPPY

/*
 * ROM-CODE kernel-loader
 */

/*
 * Absolute segment locations for code in target system EPROM
 */
#define CONFIG_ROM_SETUP_CODE 0xe000
#define CONFIG_ROM_KERNEL_CODE 0xe060

/*
 * 
 */

/*
 * Absolute segment locations for data in target system RAM
 */
#define CONFIG_ROM_SETUP_DATA 0x0060
#define CONFIG_ROM_IRQ_DATA 0x0080
#define CONFIG_ROM_KERNEL_DATA 0x0090

/*
 * Information for ROM-Image generator
 */
#define CONFIG_ROM_BASE 0xe000
#define CONFIG_ROM_CHECKSUM_SIZE (64)
#undef  CONFIG_ROM_BOOTABLE_BY_RESET
#define CONFIG_ROM_RESET_ADDRESS 0x0003
#undef  CONFIG_ROM_ADD_BIOS_IMAGE
#define CONFIG_ROM_BIOS_MODULE "bios/bioscode.bin"
#define CONFIG_ROM_BIOS_MODULE_ADDR 0xf000

/*
 * Generate debug code and information
 */
#undef  CONFIG_ROM_DEBUG

/*
 * EPROM Simulator
 */
#undef  CONFIG_ROM_USE_SIMULATOR
#define CONFIG_ROM_SIMULATOR_PROGRAM "/usr/bin/simu -t18 Image"

/*
 * 286 Protected Mode Support
 */
#undef  CONFIG_286PMODE

/*
 * Driver Support
 */

/*
 * Character device drivers
 */

/*
 * Select a console driver
 */
#define CONFIG_CONSOLE_DIRECT 1
#undef  CONFIG_CONSOLE_BIOS
#undef  CONFIG_CONSOLE_SERIAL
#define CONFIG_DCON_VT52 1
#define CONFIG_DCON_ANSI 1
#undef  CONFIG_DCON_ANSI_PRINTK
#define CONFIG_KEYMAP_US 1
#undef  CONFIG_KEYMAP_BE
#undef  CONFIG_KEYMAP_UK
#undef  CONFIG_KEYMAP_DE
#undef  CONFIG_KEYMAP_DV
#undef  CONFIG_KEYMAP_ES
#undef  CONFIG_KEYMAP_FR
#undef  CONFIG_KEYMAP_SE
#define CONFIG_DCON_KRAW 1

/*
 * Other character devices
 */
#define CONFIG_CHAR_DEV_RS 1
#define CONFIG_CHAR_DEV_LP 1
#define CONFIG_CHAR_DEV_MEM 1
#define CONFIG_PSEUDO_TTY 1
#undef  CONFIG_DEV_META

/*
 * Block device drivers
 */
#define CONFIG_BLK_DEV_BIOS 1
#define CONFIG_BLK_DEV_BFD 1
#define CONFIG_BLK_DEV_BHD 1
#undef  CONFIG_BLK_DEV_FD
#undef  CONFIG_BLK_DEV_HD
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_DMA
#define CONFIG_GENDISK 1

/*
 * Additional block devices
 */
#undef  CONFIG_BLK_DEV_RAM

/*
 * Block device options
 */
#define CONFIG_BLK_DEV_CHAR 1

/*
 * Filesystems
 */
#define CONFIG_MINIX_FS 1
#undef  CONFIG_ROMFS_FS
#undef  CONFIG_ELKSFS_FS

/*
 * General filesystem settings
 */
#undef  CONFIG_FULL_VFS
#define CONFIG_FS_EXTERNAL_BUFFER 1
#define CONFIG_PIPE 1

/*
 * Executable file formats
 */
#define CONFIG_EXEC_MINIX 1
#undef  CONFIG_EXEC_MSDOS

/*
 * Network Support
 */
#define CONFIG_SOCKET 1
#undef  CONFIG_NANO
#define CONFIG_INET 1
#define CONFIG_INET_STATUS 1
#undef  CONFIG_UNIX

/*
 *   Advanced socket options
 */
#undef  CONFIG_SOCK_CLIENTONLY

/*
 * Kernel hacking
 */
#undef  CONFIG_STRACE
#undef  CONFIG_OPT_SMALL
