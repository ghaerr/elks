#ifndef __LINUXMT_MSDOS_H
#define __LINUXMT_MSDOS_H

/*
 *	MSDOS binary formats
 */

#define MSDOS_MAGIC		0x4d5a	/* MZ */

struct msdos_exec_hdr {
    __u16			magic;
};

#endif
