#ifndef __LINUX_MSDOS_H
#define __LINUX_MSDOS_H

/*
 *	MSDOS binary formats
 */

#ifndef EXEC_HEADER_SIZE
#define EXEC_HEADER_SIZE	32
#endif

struct msdos_exec_hdr
{
	unsigned int magic; /* FIXME - is int 16bit? */
}

#endif
