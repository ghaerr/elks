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

#define LP_PBUSY	0x80 /* active low: busy, cannot take data */
#define LP_PACK		0x40 /* active low: character received */
#define LP_POUTPA	0x20 /* active high: paper empty */
#define LP_PSELECD	0x10 /* active high: printer online */
#define LP_PERRORP	0x08 /* active low: error condition */

/* control port defines */

#define LP_CONTROL(val, p)	\
	outb_p((unsigned char) (val), (void *) ((p)->io + CONTROL));

#define LP_SELECT	0x08 /* active low: printer is selected */
#define LP_INIT		0x04 /* active low: reset */
#define LP_AUTOLF	0x02 /* active low: auto insert lf for cf */
#define LP_STROBE	0x01 /* active low: valid data is asserted */

/* other defines */

/* value written to port when probing */
#define LP_DUMMY	0x00

/* microseconds to assert reset */
#define LP_RESET_WAIT	50
/* loops to busy-wait for printer to be ready to receive char */
#define LP_CHAR_WAIT	1000
/* jiffies to wait for space in printer buffer */
#define LP_TIME_WAIT	(HZ / 10)

/* defines max number of ports */
#ifndef BIOS_PORTS

/* when this is set to 3 lp probes for port only on first three i/o addresses */
#define LP_PORTS	3

#else

#define LP_PORTS	4

#endif

#define LP_DEVICE_NAME	"lp"

#endif
