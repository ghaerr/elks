#ifndef _LINUXMT_LP_H
#define _LINUXMT_LP_H

/*
 * lp.h header for ELKS kernel
 * Copyright (C) 1997 Blaz Antonic
 */

/* port.flags defines */

#define LP_EXIST	0x01
#define LP_BUSY		0x02

/* status port defines */

#define LP_OUTOFPAPER	0x20
#define LP_SELECTED	0x10

/* control port defines */

#define LP_SELECT	0x08
#define LP_INIT		0x04
#define LP_STROBE	0x01

/* define offsets from base port address for status and control port */

#define STATUS		1
#define CONTROL		2

/* other defines */

/* defines delay which should represent 5 us in while loop; 0 in lp.h in Linux */
#define LP_WAIT		0

/* defines max number of ports */
#define LP_PORTS	3

#define LP_DEVICE_NAME	"lp"

#endif _LINUXMT_LP_H
