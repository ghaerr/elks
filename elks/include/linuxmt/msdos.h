#ifndef __LINUX_MSDOS_H__
#define __LINUX_MSDOS_H__

/*
 *	MSDOS binary formats
 */

#ifndef EXEC_HEADER_SIZE
#define EXEC_HEADER_SIZE	32
#endif
#ifndef MSDOS_MAGIC
#define MSDOS_MAGIC		0x4d5a	/* MZ */
#endif

struct msdos_exec_hdr
{
    unsigned int magic;		/* FIXME - is int 16bit? */
};

#endif
