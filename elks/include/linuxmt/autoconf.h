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
#define CONFIG_IBMPC_CLONE 1

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
#define CONFIG_NANO 1
#undef  CONFIG_INET
#undef  CONFIG_UNIX

/*
 *   Advanced socket options
 */
#undef  CONFIG_SOCK_CLIENTONLY
#define CONFIG_SOCK_STREAMONLY 1

/*
 * Kernel hacking
 */
#undef  CONFIG_STRACE
#undef  CONFIG_OPT_SMALL
