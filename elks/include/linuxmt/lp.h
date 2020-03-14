#ifndef __LINUXMT_LP_H
#define __LINUXMT_LP_H

/* lp.h header for ELKS kernel
 * Copyright (C) 1997 Blaz Antonic
 */

/* Do not probe for ports; use values set by BIOS at startup instead */
#define BIOS_PORTS

/* port.flags defines */

#define LP_EXIST	0x01
#define LP_BUSY		0x04

/* define offsets from base port address for status and control port */

#define STATUS		1
#define CONTROL		2

/* status port defines */

#define LP_STATUS(p)	inb_p(p->io + STATUS)

#define LP_OUTOFPAPER	0x20
#define LP_SELECTED	0x10
#define LP_ERROR	0x08

/* control port defines */

#define LP_CONTROL(val, p)	\
	outb_p((unsigned char) (val), (void *) ((p)->io + CONTROL));

#define LP_SELECT	0x08
#define LP_INIT		0x04
#define LP_STROBE	0x01

/* other defines */

/* value written to port when probing */
#define LP_DUMMY	0x00

/* defines delay which should represent 5 us in while loop; 0 in lp.h in Linux */
#define LP_RESET_WAIT	100
#define LP_CHAR_WAIT	100
#define LP_STROBE_WAIT	50

/* defines max number of ports */
#ifndef BIOS_PORTS

/* when this is set to 3 lp probes for port only on first three i/o addresses */
#define LP_PORTS	3

#else

#define LP_PORTS	4

#endif

#define LP_DEVICE_NAME	"lp"

#endif
