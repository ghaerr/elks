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

#endif
