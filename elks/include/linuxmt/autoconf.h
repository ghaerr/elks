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
#undef  CONFIG_ARCH_PC_AUTO
#define CONFIG_ARCH_SIBO 1
#undef  CONFIG_ARCH_PC_XT
#undef  CONFIG_ARCH_PC_AT
#undef  CONFIG_MODULE
#undef  CONFIG_SHLIB

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
#undef  CONFIG_NOFS
#undef  CONFIG_EXEC_ROM

/*
 * Character device drivers
 */

/*
 * Select a console driver
 */
#define CONFIG_SIBO_CONSOLE_DIRECT 1
#undef  CONFIG_CONSOLE_SERIAL
#undef  CONFIG_SIBO_VIRTUAL_CONSOLE
#undef  CONFIG_SIBO_CONSOLE_ECHO
#undef  CONFIG_SIBO_3C_KEYMAP
#undef  CONFIG_SIBO_3MX_KEYMAP
#define CONFIG_SIBO_DEFAULT_KEYMAP 1

/*
 * Other character devices
 */
#define CONFIG_CHAR_DEV_MEM 1
#undef  CONFIG_PSEUDO_TTY
#undef  CONFIG_DEV_META

/*
 * Block device drivers
 */
#define CONFIG_BLK_DEV_SSD 1
#undef  CONFIG_BLK_DEV_ID

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
#undef  CONFIG_SOCKET
