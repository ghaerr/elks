/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED

/*
 * Just accept the defaults unless you know what you are doing
 */

/*
 * General setup
 */
#define CONFIG_ARCH_PC_AUTO 1
#undef  CONFIG_ARCH_SIBO
#undef  CONFIG_ARCH_PC_XT
#undef  CONFIG_ARCH_PC_AT
#undef  CONFIG_ARCH_PC_MCA
#undef  CONFIG_MODULE
#undef  CONFIG_SHLIB
#undef  CONFIG_COMPAQ_FAST

/*
 * ROM-CODE kernel-loader
 */
#undef  CONFIG_ROMCODE

/*
 * 286 protected mode support
 */
#undef  CONFIG_286PMODE

/*
 * Kernel hacking
 */
#undef  CONFIG_STRACE
#undef  CONFIG_OPT_SMALL

/*
 * Embedded systems
 */

/*
 * Currently unimplemented features for use in embedded systems
 */
#undef  CONFIG_EXEC_ROM

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
#undef  CONFIG_BE_KEYMAP
#undef  CONFIG_FR_KEYMAP
#undef  CONFIG_UK_KEYMAP
#undef  CONFIG_ES_KEYMAP
#undef  CONFIG_DE_KEYMAP
#undef  CONFIG_SE_KEYMAP
#define CONFIG_DEFAULT_KEYMAP 1
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
#undef  CONFIG_FS_RO

/*
 * Executable file formats
 */
#define CONFIG_EXEC_MINIX 1
#undef  CONFIG_EXEC_MSDOS

/*
 * Networking
 */
#define CONFIG_SOCKET 1
#undef  CONFIG_UNIX
#define CONFIG_NANO 1
#undef  CONFIG_INET

/*
 *   Advance socket options
 */
#undef  CONFIG_SOCK_CLIENTONLY
#define CONFIG_SOCK_STREAMONLY 1
