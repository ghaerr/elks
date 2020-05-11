#ifndef __LINUXMT_MINIX_H
#define __LINUXMT_MINIX_H

/*
 *	Minix binary formats
 */

#define RUNNABLE_HEADER_SIZE	32

/* Values for the type field of minix_exec_header */

#define MINIX_COMBID	0x04100301UL
/* Separate I/D (0x20), not marked executable, but actually executable ---
   this was the header value used in actual Minix 1 and 2 a.out's */
#define MINIX_SPLITID	0x04200301UL
/* Executable (0x10) with separate I/D (0x20) */
#define MINIX_SPLITID_AHISTORICAL 0x04300301UL
#define MINIX_S_SPLITID	0x04600301UL
#define MINIX_DLLID	0x04A00301UL

struct minix_exec_hdr {
    unsigned long	type;
    unsigned long	hlen;
    unsigned long	tseg;
    unsigned long	dseg;
    unsigned long	bseg;
    unsigned long	entry;
    unsigned long	chmem;
    unsigned long	unused2;
};

struct elks_supl_hdr {
    /* optional fields */
    long		msh_trsize;	/* text relocation size */
    long		msh_drsize;	/* data relocation size */
    long		msh_tbase;	/* text relocation base */
    long		msh_dbase;	/* data relocation base */
    /* even more optional fields --- for ELKS medium memory model support */
    unsigned short	esh_ftseg;	/* far text size */
    unsigned long	esh_ftrsize;	/* far text relocation size */
    unsigned short	unused3;
    unsigned long	unused4, unused5;
};

struct minix_reloc {
    unsigned long	r_vaddr;	/* address of place within section */
    unsigned short	r_symndx;	/* index into symbol table */
    unsigned short	r_type;		/* relocation type */
};

/* r_type values */
#define R_SEGWORD	80

/* special r_symndx values */
#define S_ABS		((unsigned short)-1U)
#define S_TEXT		((unsigned short)-2U)
#define S_DATA		((unsigned short)-3U)
#define S_BSS		((unsigned short)-4U)
/* for ELKS medium memory model support */
#define S_FTEXT		((unsigned short)-5U)

#endif
