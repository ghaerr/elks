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

/* Default data allocations*/
#define INIT_HEAP  4096		/* Default heap*/
#define INIT_STACK 4096		/* Default stack*/

typedef unsigned int	uint32;
typedef unsigned short	uint16;
typedef unsigned char	uint8;

struct minix_exec_hdr {
    uint32	type;
    uint8	hlen;		// 0x04
    uint8	reserved1;
    uint8	version;
    uint32	tseg;		// 0x08
    uint32	dseg;		// 0x0c
    uint32	bseg;		// 0x10
    uint32	entry;
    uint16	chmem;
    uint16	minstack;
    uint32	syms;
};

struct elks_supl_hdr {
    /* optional fields */
    uint32	msh_trsize;	/* text relocation size */	// 0x20
    uint32	msh_drsize;	/* data relocation size */	// 0x24
    uint32	msh_tbase;	/* text relocation base */
    uint32	msh_dbase;	/* data relocation base */
    /* even more optional fields --- for ELKS medium memory model support */
    uint32	esh_ftseg;	/* far text size */		// 0x30
    uint32	esh_ftrsize;	/* far text relocation size */	// 0x34
    /* optional fields for compressed binaries */
    uint16	esh_compr_tseg;	/* compressed tseg size */
    uint16	esh_compr_dseg;	/* compressed dseg size* */
    uint16	esh_compr_ftseg;/* compressed ftseg size*/
    uint16	esh_reserved;
};

struct minix_reloc {
    uint32	r_vaddr;	/* address of place within section */
    uint16	r_symndx;	/* index into symbol table */	// 0x04
    uint16	r_type;		/* relocation type */		// 0x06
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

/* header sizes */
/* executable with no far text, no relocations */
#define EXEC_MINIX_HDR_SIZE	sizeof(struct minix_exec_hdr)
/* executable with relocations, no far text (?) */
#define SUPL_RELOC_HDR_SIZE	offsetof(struct elks_supl_hdr, esh_ftseg)
#define EXEC_RELOC_HDR_SIZE	(EXEC_MINIX_HDR_SIZE + SUPL_RELOC_HDR_SIZE)
/* executable with far text, and optionally relocations */
#define SUPL_FARTEXT_HDR_SIZE	sizeof(struct elks_supl_hdr)
#define EXEC_FARTEXT_HDR_SIZE	(EXEC_MINIX_HDR_SIZE + SUPL_FARTEXT_HDR_SIZE)

#endif
