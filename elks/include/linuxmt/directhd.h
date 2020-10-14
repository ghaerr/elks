/* directhd.h header for ELKS kernel - Copyright (C) 1998 Blaz Antonic
 */

#ifndef __LINUXMT_DIRECTHD_H
#define __LINUXMT_DIRECTHD_H

/* define offsets from base port address */
#define DIRECTHD_ERROR 1
#define DIRECTHD_SEC_COUNT 2
#define DIRECTHD_SECTOR 3
#define DIRECTHD_CYLINDER_LO 4
#define DIRECTHD_CYLINDER_HI 5
#define DIRECTHD_DH 6
#define DIRECTHD_STATUS 7
#define DIRECTHD_COMMAND 7

/* define drive masks */
#define DIRECTHD_DRIVE0 0xa0
#define DIRECTHD_DRIVE1 0xb0

/* define drive commands */
#define DIRECTHD_DRIVE_ID 0xec	/* drive id */
#define DIRECTHD_READ 0x20	/* read with retry */
#define DIRECTHD_WRITE 0x30	/* write with retry */

/* other definitions */
#define MAX_DRIVES 4		/* 2 per i/o channel and 2 i/o channels */

#if 0
#define DIRECTHD_DEVICE_NAME	"dhd"
#endif

#endif
