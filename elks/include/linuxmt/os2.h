#ifndef OS2_H_
#define OS2_H_
/*
 * OS/2 NE and DOS MZ executable header layout
 *
 * 28 Jun 2024 Greg Haerr
 */

#include <stdint.h>

#define MZMAGIC     0x5a4d      /* "MZ" magic number for DOS MZ executables */

struct dos_exec_hdr {           /* DOS MZ Executable header */
    uint16_t magic;             /* Magic number                     0x00 */
    uint16_t bytes_last_page;   /* Bytes on last page of file       0x02 */
    uint16_t num_pages;         /* Pages in file                    0x04 */
    uint16_t num_relocs;        /* Relocations                      0x06 */
    uint16_t hdr_paras;         /* Size of header in paragraphs     0x08 */
    uint16_t min_paras;         /* Minimum extra paragraphs needed */
    uint16_t max_paras;         /* Maximum extra paragraphs needed */
    uint16_t reg_ss;            /* Initial (relative) SS value      0x0e */
    uint16_t reg_sp;            /* Initial SP value                 0x10 */
    uint16_t chksum;            /* Checksum */
    uint16_t reg_ip;            /* Initial IP value                 0x14 */
    uint16_t reg_cs;            /* Initial (relative) CS value      0x16 */
    uint16_t reloc_table_offset;/* File address of relocation table 0x18 */
    uint16_t overlay_num;       /* Overlay number */
    uint16_t reserved1[4];      /* Reserved words */
    uint16_t oemid;             /* OEM identifier */
    uint16_t oeminfo;           /* OEM information, oemid specific */
    uint16_t reserved2[10];     /* Reserved words */
    uint32_t ext_hdr_offset;    /* File address of new exe header   0x3C */
};

struct dos_reloc {              /* DOS relocation table entry */
    uint16_t r_offset;          /* Offset of segment to reloc from r_seg */
    uint16_t r_seg;             /* Segment relative to load segment */
};

#define NEMAGIC    0x454e       /* "NE" magic number for OS/2 NE executables */

/* Program flags */
#define NEPRG_FLG_MULTIPLEDATA  0x02    /* Instanced auto data segment */
#define NEPRG_FLG_PROTMODE      0x80    /* Protected mode only */

/* Target OS */
#define NETARGET_OS2            0x01    /* OS/2 target */

struct ne_exec_hdr {            /* OS/2 New Executable header */
    uint16_t magic;             /* NE signature                     0x00 */
    uint8_t linker_version;     /* Linker major version */
    uint8_t linker_revision;    /* Linker minor version */
    uint16_t entry_table_offset;
    uint16_t entry_table_size;
    uint32_t crc32;             /* File load CRC, may be 0 */
    uint8_t program_flags;      /* Flags                            0x0C */
    uint8_t application_flags;
    uint16_t auto_data_segment; /* Auto data segment index          0x0E */
    uint16_t heap_size;         /* Initial local heap size          0x10 */
    uint16_t stack_size;        /* Initial stack size               0x12 */
    uint16_t reg_ip;            /* IP entry point                   0x14 */
    uint16_t reg_cs;            /* CS index into segment table      0x16 */
    uint16_t reg_sp;            /* Initial SP                       0x18 */
    uint16_t reg_ss;            /* SS index into segment table      0x1A */
    uint16_t num_segments;      /* Segment count                    0x1C */
    uint16_t num_modules;       /* Number of module refs (DLLs)     0x1E */
    uint16_t non_resident_name_table_size;
    uint16_t segment_table_offset;          /* Segment table        0x22 */
    uint16_t resource_table_offset;
    uint16_t resident_name_table_offset;
    uint16_t module_reference_table_offset; /* Module ref table offset */
    uint16_t imported_names_table_offset;
    uint32_t non_resident_name_table_offset;
    uint16_t num_movable_entries;
    uint16_t file_alignment_shift_count;    /* 0=9, default 512-byte pages 0x32 */
    uint16_t num_resource_entries;
    uint8_t target_os;          /* Target OS: 0=Unknown, 1=OS/2     0x36 */
    uint8_t os2_flags;          /* Other OS/2 flags */
    uint16_t return_thunks_offset;
    uint16_t segment_reference_thunks_offset;
    uint16_t min_code_swap_size;
    uint8_t windows_revision;   /* Expected Windows minor version   0x3E */
    uint8_t windows_version;    /* Expected Windows major version   0x3F */
};

/* Segment table entry flag bits */
#define NESEG_TYPE_MASK 0x0007  /* Segment type field */
#define NESEG_CODE      0x0000  /* Code segment */
#define NESEG_DATA      0x0001  /* Data segment */
#define NESEG_MOVEABLE  0x0010  /* Segment is not fixed */
#define NESEG_PRELOAD   0x0040  /* Preload segment */
#define NESEG_RELOCINFO 0x0100  /* Segment has relocation records */
#define NESEG_DISCARD   0xF000  /* Discard priority */

struct ne_segment {             /* NE executable segment table entry */
    uint16_t offset;            /* Logical (shifted) sector offset to segment data */
    uint16_t size;              /* Length of segment in file, 0=64K */
    uint16_t flags;             /* Flag word */
    uint16_t min_alloc;         /* Min allocation size of segment in bytes, 0=64K */
};

struct ne_reloc_num {           /* NE number relocation records */
    uint16_t num_records;       /* appears directly after segment data */
};

/*
 * OS/2 fixups are named confusingly - "source" identifies the (dest) relocation
 * being fixed up, and "target" identifies segment/offset being referenced.
 */

/* src_type */
#define NEFIXSRC_TYPE_MASK  0x000F  /* "source" type field */
#define NEFIXSRC_LOBYTE     0x0000
#define NEFIXSRC_SEGMENT    0x0002  /* Segment fixup */
#define NEFIXSRC_FARADDR    0x0003  /* 32-bit pointer fixup */
#define NEFIXSRC_OFFSET     0x0005  /* 16-bit pointer fixup */

/* flags */
#define NEFIXFLG_TARGET_MASK 0x0003 /* "target" type field */
#define NEFIXFLG_INTERNALREF 0x0000 /* Internal reference (standard fixup) */
#define NEFIXFLG_IMPORDINAL  0x0001
#define NEFIXFLG_IMPNAME     0x0002
#define NEFIXFLG_OSFIXUP     0x0003
#define NEFIXFLG_ADDITIVE    0x0004 /* Add target value to source */

struct ne_reloc {               /* NE segment relocation entry */
    uint8_t src_type;           /* Source type */
    uint8_t flags;              /* Flags byte */
    uint16_t src_chain;         /* Offset in current segment of source chain */
    /* INTERNALREF target info */
    uint8_t segment;            /* Target segment number */
    uint8_t zero;               /* Zero byte */
    uint16_t offset;            /* Offset into target segment */
};

#endif /* OS2_H_ */
