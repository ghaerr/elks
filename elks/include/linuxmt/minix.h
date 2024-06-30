#ifndef __LINUXMT_MINIX_H
#define __LINUXMT_MINIX_H

/*
 *      Minix binary formats
 */

#include <stdint.h>             /* this file included by host, kernel & libc */

/* Values for the type field of minix_exec_header */

#define AOUTMAGIC       0x0301

#define MINIX_COMBID    0x04100301UL
/* Separate I/D (0x20), not marked executable, but actually executable ---
   this was the header value used in actual Minix 1 and 2 a.out's */
#define MINIX_SPLITID   0x04200301UL
/* Executable (0x10) with separate I/D (0x20) */
#define MINIX_SPLITID_AHISTORICAL 0x04300301UL
#define MINIX_S_SPLITID 0x04600301UL
#define MINIX_DLLID     0x04A00301UL

/* Default data allocations*/
#define INIT_HEAP  4096         /* Default heap*/
#define INIT_STACK 4096         /* Default stack*/

struct minix_exec_hdr {
    uint32_t    type;
    uint8_t     hlen;           // 0x04
    uint8_t     reserved1;
    uint8_t     version;
    uint32_t    tseg;           // 0x08
    uint32_t    dseg;           // 0x0c
    uint32_t    bseg;           // 0x10
    uint32_t    entry;
    uint16_t    chmem;
    uint16_t    minstack;
    uint32_t    syms;
};

struct elks_supl_hdr {
    /* optional fields */
    uint32_t    msh_trsize;     /* text relocation size */      // 0x20
    uint32_t    msh_drsize;     /* data relocation size */      // 0x24
    uint32_t    msh_tbase;      /* text base address (low stack only) */
    uint32_t    msh_dbase;      /* data base address (low stack only) */
    /* even more optional fields --- for ELKS medium memory model support */
    uint32_t    esh_ftseg;      /* far text size */             // 0x30
    uint32_t    esh_ftrsize;    /* far text relocation size */  // 0x34
    /* optional fields for compressed binaries */
    uint16_t    esh_compr_tseg; /* compressed tseg size */
    uint16_t    esh_compr_dseg; /* compressed dseg size* */
    uint16_t    esh_compr_ftseg;/* compressed ftseg size*/
    uint16_t    esh_reserved;
};

struct minix_reloc {
    uint32_t    r_vaddr;        /* address of place within section */
    uint16_t    r_symndx;       /* index into symbol table */   // 0x04
    uint16_t    r_type;         /* relocation type */           // 0x06
};

/* r_type values */
#define R_SEGWORD       80

/* special r_symndx values */
#define S_ABS           ((unsigned short)-1U)
#define S_TEXT          ((unsigned short)-2U)
#define S_DATA          ((unsigned short)-3U)
#define S_BSS           ((unsigned short)-4U)
/* for ELKS medium memory model support */
#define S_FTEXT         ((unsigned short)-5U)

/* header sizes */
/* executable with no far text, no relocations */
#define EXEC_MINIX_HDR_SIZE     sizeof(struct minix_exec_hdr)
/* executable with relocations, no far text (?) */
#define SUPL_RELOC_HDR_SIZE     offsetof(struct elks_supl_hdr, esh_ftseg)
#define EXEC_RELOC_HDR_SIZE     (EXEC_MINIX_HDR_SIZE + SUPL_RELOC_HDR_SIZE)
/* executable with far text, and optionally relocations */
#define SUPL_FARTEXT_HDR_SIZE   sizeof(struct elks_supl_hdr)
#define EXEC_FARTEXT_HDR_SIZE   (EXEC_MINIX_HDR_SIZE + SUPL_FARTEXT_HDR_SIZE)

/* elks/fs/exodecr.c - minix binary decompressor */
extern unsigned int decompress(char *buf, unsigned int seg, unsigned int orig_size,
    unsigned int compr_size, int safety);

#endif
