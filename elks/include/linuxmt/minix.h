#ifndef __LINUX_MINIX_H
#define __LINUX_MINIX_H

/*
 *	Minix binary formats
 */
 
#define EXEC_HEADER_SIZE	32

struct minix_exec_hdr
{
	unsigned long type;
#define MINIX_COMBID	0x04100301L
#define MINIX_SPLITID	0x04200301L	
#define MINIX_S_SPLITID	0x04600301L	
#define MINIX_DLLID	0x04A00301L	
	unsigned long hlen;
	unsigned long tseg;
	unsigned long dseg;
	unsigned long bseg;
	unsigned long unused;
	unsigned long chmem;
	unsigned long unused2; 
};

#endif
