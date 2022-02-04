/* This header file contains the definitions that relate to the ELKS
 * boot variables, ready for use by any file needing access to them.
 */

#ifndef __LINUXMT_BOOT_H
#define __LINUXMT_BOOT_H

/* Root flags */

#define RF_NONE 	0
#define RF_RO		1

#ifdef CONFIG_ROOT_READONLY
#define ROOTFLAGS	RF_RO
#else
#define ROOTFLAGS	RF_NONE
#endif

/* ELKS flags */

#define EF_NONE		0
#define EF_AS_BLOB	0x8000		/* says that setup and kernel are
					   loaded as one blob at SETUPSEG:0,
					   and setup may need to move kernel
					   to SYSSEG:0 */
#define EF_BIOS_DEV_NUM	0x4000		/* says that root_dev does not give
					   a <major, minor> block device
					   number, but only a BIOS drive
					   number (and possibly other
					   information); setup or kernel
					   should use this drive number to
					   figure out the correct root
					   device */

#define ELKSFLAGS	EF_NONE

/* If we are not building the (dummy) boot sector (elks/elks/arch/i86/boot/
   {bootsect.S, netbootsect.S}) at the start of /linux, then define
   constants giving the offsets of various fields within /linux.

   The fields in /linux are mainly based on an old version of the Linux/x86
   Boot Protocol (https://www.kernel.org/doc/html/latest/x86/boot.html).
   Fields which are specific to ELKS are indicated below.  */

#ifndef DUMMYBOOT
#define screen_cols	7		/* byte screen width*/
#define screen_lines	14		/* byte screen height*/
#define cpu_type	0x20		/* byte cpu type*/
#define mem_kbytes	0x2a		/* word base memory size in Kbytes*/
#define proc_name	0x30		/* 16 bytes processor name string*/
#define cpu_id		0x50		/* 13 bytes cpu id string*/
#define part_offset	0x1e2		/* long sector offset of booted partition*/
#define elks_magic	0x1e6		/* long "ELKS" (45 4c 4b 53) checked by bootsect.S*/
#define setup_sects	0x1f1		/* byte 512-byte sectors used by setup.S*/
#define syssize		0x1f4		/* word paragraph kernel size used by setup.S*/
#define elks_flags	0x1f6		/* word 16-bit ELKS flags, BLOB and BIOS_DRV*/
#define root_dev	0x1fc		/* word BIOS drive or kdev_t ROOT_DEV*/
#define boot_flag	0x1fe		/* word constant AA55h*/
#endif

#endif
