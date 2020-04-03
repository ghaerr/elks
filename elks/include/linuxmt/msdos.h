#ifndef __LINUXMT_MSDOS_H
#define __LINUXMT_MSDOS_H

/*
 *	MSDOS binary formats
 */

#ifndef RUNNABLE_HEADER_SIZE
#define RUNNABLE_HEADER_SIZE	32
#endif

#ifndef MSDOS_MAGIC
#define MSDOS_MAGIC		0x4d5a	/* MZ */
#endif

struct msdos_exec_hdr {
    __u16			magic;
};

#endif
