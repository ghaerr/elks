/* This header file contains the definitions that relate to the ELKS
 * boot variables, ready for use by any file needing access to them.
 */

#ifndef LX86_LINUXMT_BOOT_H
#define LX86_LINUXMT_BOOT_H

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

#define ELKSFLAGS	0

#endif
