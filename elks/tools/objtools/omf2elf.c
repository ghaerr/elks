/* 24 May 2024 from https://github.com/pts/minilibc686, licensed under GPL v2.0 */
/*
 * omf2elf.c: convert i386 OMF .obj object file to i386 ELF relocatable .o object file
 * by pts@fazekas.hu at Thu Jun  1 14:22:39 CEST 2023
 *
 * Compile: minicc -o tools/omf2elf tools/omf2elf.c
 *
 * This program was only tested on OMF .obj files created by OpenWatcom
 * wcc386. It will probably fail on the output of assemblers or other C
 * compilers. (The output of NASM on test/demo_obj.nasm works though,
 * because it contains the expected segment definitions.)
 *
 * This program is written in standard C (C89 and later, also C++98 and
 * later). It works on any little-endian system with sizeof(int) >= 32 and
 * correct struct alignment for <elf.h>. It has been tested with GCC (both C
 * and C++), Clang (both C and C++), TinyCC (C) and OpenWatcom (C)
 * compilers.
 *
 * 24 May 2024 Greg Haerr - Added 16-bit conversion capabilities for ELKS
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__i386) || defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__386) || \
    defined(__X86_64__) || defined(_M_I386) || defined(_M_I86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86) || defined(__386) || \
    defined(__X86__) || defined(__I86__) || defined(_M_I86) || defined(_M_I8086) || defined(_M_I286)
#  define IS_X86 1
#endif

/* --- ELF file format. */

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct {
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf32_Half	e_type;			/* Object file type */
  Elf32_Half	e_machine;		/* Architecture */
  Elf32_Word	e_version;		/* Object file version */
  Elf32_Addr	e_entry;		/* Entry point virtual address */
  Elf32_Off	e_phoff;		/* Program header table file offset */
  Elf32_Off	e_shoff;		/* Section header table file offset */
  Elf32_Word	e_flags;		/* Processor-specific flags */
  Elf32_Half	e_ehsize;		/* ELF header size in bytes */
  Elf32_Half	e_phentsize;		/* Program header table entry size */
  Elf32_Half	e_phnum;		/* Program header table entry count */
  Elf32_Half	e_shentsize;		/* Section header table entry size */
  Elf32_Half	e_shnum;		/* Section header table entry count */
  Elf32_Half	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

#define ELFCLASS32	1		/* 32-bit objects */

#define ET_REL		1		/* Relocatable file */

#define EM_386		 3	/* Intel 80386 */

/* Section header.  */

typedef struct {
  Elf32_Word	sh_name;		/* Section name (string tbl index) */
  Elf32_Word	sh_type;		/* Section type */
  Elf32_Word	sh_flags;		/* Section flags */
  Elf32_Addr	sh_addr;		/* Section virtual addr at execution */
  Elf32_Off	sh_offset;		/* Section file offset */
  Elf32_Word	sh_size;		/* Section size in bytes */
  Elf32_Word	sh_link;		/* Link to another section */
  Elf32_Word	sh_info;		/* Additional section information */
  Elf32_Word	sh_addralign;		/* Section alignment */
  Elf32_Word	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;

#define SHT_PROGBITS	  1		/* Program data */
#define SHT_SYMTAB	  2		/* Symbol table */
#define SHT_STRTAB	  3		/* String table */
#define SHT_NOBITS	  8		/* Program space with no data (bss) */
#define SHT_REL		  9		/* Relocation entries, no addends */

#define SHF_WRITE	     (1 << 0)	/* Writable */
#define SHF_ALLOC	     (1 << 1)	/* Occupies memory during execution */
#define SHF_EXECINSTR	     (1 << 2)	/* Executable */
#define SHF_MERGE	     (1 << 4)	/* Might be merged */
#define SHF_STRINGS	     (1 << 5)	/* Contains nul-terminated strings */

/* Symbol table entry.  */

typedef struct {
  Elf32_Word	st_name;		/* Symbol name (string tbl index) */
  Elf32_Addr	st_value;		/* Symbol value */
  Elf32_Word	st_size;		/* Symbol size */
  unsigned char	st_info;		/* Symbol type and binding */
  unsigned char	st_other;		/* Symbol visibility */
  Elf32_Section	st_shndx;		/* Section index */
} Elf32_Sym;

#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL      1               /* Global symbol */
#define STB_WEAK        2               /* Weak symbol */

#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */

#define ELF32_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xf)
#define ELF32_ST_INFO(bind, type)	(((bind) << 4) + ((type) & 0xf))

#define SHN_UNDEF	0		/* Undefined section */
#define SHN_ABS		0xfff1		/* Associated symbol is absolute */

/* Relocation table entry. */

typedef struct {
  Elf32_Addr	r_offset;		/* Address */
  Elf32_Word	r_info;			/* Relocation type and symbol index */
} Elf32_Rel;

#define ELF32_R_SYM(val)		((val) >> 8)
#define ELF32_R_TYPE(val)		((val) & 0xff)
#define ELF32_R_INFO(sym, type)		(((sym) << 8) + ((type) & 0xff))

#define R_386_32	   1		/* Direct 32 bit  */
#define R_386_PC32	   2		/* PC relative 32 bit */
#define R_386_16	  20		/* Direct 16 bit */
#define R_386_PC16	  21		/* PC relative 16 bit */

/* --- OMF file format.
 *
 * See details at: http://www.azillionmonkeys.com/qed/Omfg.pdf
 */

#define THEADR 0x80
#define MODEND 0x8a  /* Emitted by OpenWatcom. */
#define MODEND386 0x8b  /* Emitted by NASM (!). */
#define COMENT 0x88
#define EXTDEF 0x8c
#define PUBDEF 0x90  /* Emitted by NASM. */
#define PUBDEF386 0x91  /* Emitted by OpenWatcom. */
#define LNAMES 0x96
#define SEGDEF 0x98  /* Emitted by NASM. */
#define SEGDEF386 0x99  /* Emitted by OpenWatcom. */
#define GRPDEF 0x9a
#define FIXUPP 0x9c
#define FIXUPP386 0x9d
#define LEDATA 0xa0  /* Emitted by NASM. */
#define LEDATA386 0xa1  /* Emitted by OpenWatcom. */
#define LIDATA 0xa2
#define LIDATA386 0xa3  /* Emitted by OpenWatcom for long runs of 0. */
#define LPUBDEF 0xb6
#define LPUBDEF386 0xb7
#define LEXTDEF 0xb4
#define LINNUM386 0x95

static uint16_t g16(const unsigned char *p) {  /* get_u16_le(...). */
#if IS_X86  /* Unaligned read. */
  return *(const uint16_t*)p;
#else
  return p[0] | p[1] << 8;
#endif
}

static uint32_t g32(const unsigned char *p) {  /* get_u32_le(...). */
#if IS_X86  /* Unaligned read. */
  return *(const uint32_t*)p;
#else
  return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
#endif
}

static void add32(unsigned char *p, uint32_t v) {
#if IS_X86  /* Unaligned write. */
  *(uint32_t*)p += v;
#else
  v += g32(p);
  *p++ = v; v >>= 8;
  *p++ = v; v >>= 8;
  *p++ = v; v >>= 8;
  *p++ = v;
#endif
}

static void add16(unsigned char *p, uint32_t v) {
#if IS_X86  /* Unaligned write. */
  *(uint16_t*)p += v;
#else
  v += g16(p);
  *p++ = v; v >>= 8;
  *p++ = v; v >>= 8;
#endif
}

/* --- */

/* It doesn't work with smaller int sizes, some `int' and `unsigned' variables are too small. */
typedef char static_assert_sizeof_int[sizeof(int) >= 4 ? 1 : -1];

#if IS_X86 && defined(__MINILIBC686__)  /* minilibc686 has the malloc_simple_unaligned(...) function in <stdlib.h>. */
#  define my_malloc(size) malloc_simple_unaligned(size)  /* It may return NULL for 0. */
#else
#  define my_malloc(size) malloc(size)  /* Other architectures may not support unaligned memory access. */
#endif

static char *my_xmalloc(size_t size) {
  char *q = (char*)my_malloc(size != 0 ? size : 1);
  if (!q) {
    fprintf(stderr, "fatal: out of memory\n");
    exit(15);  /* return 15; */
  }
  return q;
}

static char *my_strdup_size(const char *p, size_t size) {
  char *q;
  if (memchr(p, '\0', size)) {
    fprintf(stderr, "fatal: NUL in string\n");
    exit(16);  /* return 16; */
  }
  q = my_xmalloc(size + 1);
  q[size] = '\0';
  return (char*)memcpy(q, p, size);
}

struct SectionInfo {
  const char *rel_section_name;
  const char *seg_name;
  const char *class_name;
  Elf32_Word sh_flags;
};

#define SECTION_TEXT 0
#define SECTION_SDATA 1
#define SECTION_RODATA 2
#define SECTION_BSS 4
#define SEG_YIB 5  /* Emitted by the patched `tools/wcc386 -ec' instead of SECTION_SDATA. */
#define SEG_YI 6  /* Emitted by the patched `tools/wcc386 -ec'. Empty. */
#define SEG_YIE 7  /* Emitted by the patched `tools/wcc386 -ec'. Empty. */
#define SECTION_COUNT 8

const struct SectionInfo section_infos[SECTION_COUNT] = {
    {".rel.text", "_TEXT", "CODE", SHF_ALLOC | SHF_EXECINSTR},
    {".rel.rodata.str1.1", "CONST", "DATA", SHF_ALLOC | SHF_MERGE | SHF_STRINGS},
    {".rel.rodata", "CONST2", "DATA", SHF_ALLOC},
    {".rel.data", "_DATA", "DATA", SHF_ALLOC | SHF_WRITE},
    {".rel.bss", "_BSS", "BSS", SHF_ALLOC | SHF_WRITE},
    {".rel.yib", "YIB", "DATA", SHF_ALLOC | SHF_MERGE | SHF_STRINGS | SHF_WRITE},
    {".rel.yi", "YI", "DATA", 0},
    {".rel.yie", "YIE", "DATA", 0},
};

struct Section {
  uint16_t seg_idx;  /* 0 by default. */
  uint32_t size;
  uint32_t rel_count;  /* Number of relocations from this section. */
  uint32_t sym_count;
  const struct SectionInfo *info;
  uint32_t file_ofs;
  uint32_t rel_file_ofs;
  struct Reloc *relocs, *relocp;
  unsigned char align;  /* 1, 2 or 4. */
  char is_rel_target;  /* Is there a relocation anywhere pointing to this section? */
  unsigned char shndx;
  unsigned char symndx;
};

struct Symbol {
  const char *name;
  uint32_t value;
  unsigned char section;
  char is_global;
  uint32_t prev_idx;  /* Initially used as an index witin pubsymbols, later used as an index within syms. */
};

struct Reloc {
  uint32_t ofs;  /* Offset within the hosting section. */
  uint32_t idx;  /* If is_extdef==0, then idx is a section_idx, otherwise idx is within extsymbols (0-based).  */
  /*unsigned char section_idx;*/  /* Implicit. */
  char is_segrel;
  char is_extdef;
};

#define SYMBOL_SECTION_EXTERN 0xff

static char is_openwatcom_libc_symbol(const char *name) {
  if (name[0] != '_' || name[1] != '_') return 0;
  name += 2;
  /* The list below is incomplete. Here is the full OpenWatcom list: __8087
   * __ASTACKPTR __ASTACKSIZ __CHK __COPY __FADD __FCMP __FCMPR __FD8 __FDA __FDC
   * __FDD __FDFS __FDI4 __FDI8 __FDIV __FDIVR __FDM __FDN __FDS __FDU4 __FDU8
   * __FDU87 __FMUL __FSA __FSC __FSD __FSFD __FSI __FSI1 __FSI2 __FSI4 __FSI8
   * __FSM __FSN __FSS __FSU1 __FSU2 __FSU4 __FSU8 __FSU87 __FSUB __FSUBR __GETDS
   * __GETMAXTHREADS __GRO __I4FD __I4FS __I8D __I8FD __I8FS __I8LS __I8M __I8RS
   * __POFF __PON __RDI4 __RDU4 __STACKLOW __STACKTOP __STK __STOSB __STOSD
   * __U4FD __U4FS __U8D __U8FD __U8FD7 __U8FS __U8FS7 __U8LS __U8M __U8RS
   */
  return strcmp(name, "CHP") == 0 ||
         strcmp(name, "STK") == 0 ||
         strcmp(name, "I8D") == 0 ||
         strcmp(name, "U8D") == 0 ||
         strcmp(name, "I8RS") == 0 ||
         strcmp(name, "U8RS") == 0 ||
         strcmp(name, "I8LS") == 0 ||
         strcmp(name, "U8LS") == 0 ||
         strcmp(name, "U8M") == 0 ||
         strcmp(name, "I8M") == 0 ||
         strcmp(name, "FSU87") == 0 ||
         strcmp(name, "FDU87") == 0 ||
         strcmp(name, "U8FD7") == 0 ||
         strcmp(name, "U8FS7") == 0 ||
         strcmp(name, "FSFD") == 0 ||
         strcmp(name, "FSI4") == 0 ||
         strcmp(name, "I4FS") == 0 ||
         strcmp(name, "FDC") == 0 ||
         strcmp(name, "FSC") == 0 ||
         strcmp(name, "I4M") == 0;
}

static unsigned char unflushed_ledata_data[0x10000], *unflushed_ledata_up;  /* OMF record data for ledata* records. */
static unsigned char unflushed_ledata_section_idx;
static uint32_t unflushed_ledata_ofs, unflushed_ledata_size, unflushed_ledata_section_file_ofs;

static int flush_ledata(int fdo, const char *elfoname) {
  if (unflushed_ledata_size > 0) {
    unflushed_ledata_ofs += unflushed_ledata_section_file_ofs;
    if ((uint32_t)lseek(fdo, unflushed_ledata_ofs, SEEK_SET) != unflushed_ledata_ofs) {  /* Usually this succeeds, and then the write fails. */
      fprintf(stderr, "fatal: error seeking in ELF .o: %s\n", elfoname);
      return 38;
    }
    if ((size_t)write(fdo, unflushed_ledata_up, unflushed_ledata_size) != unflushed_ledata_size) {
      fprintf(stderr, "fatal: error writing to ELF .o: %s\n", elfoname);
      return 39;
    }
    unflushed_ledata_size = unflushed_ledata_ofs = 0;
  }
  return 0;
}

int main(int argc, char **argv) {
  const char *arg;
  char **argp;
  FILE *f;
  int fdo = -1;
  char verbose_level = 0, do_strip_one_leading_underscore = 0;
  char do_strip_one_trailing_underscore = 0;
  char is_pass2;
  char is_fpc;
  const char *omfname;
  const char *elfoname = NULL;
  static Elf32_Ehdr ehdr = {
      /* .e_ident = */ {'\x7f', 'E', 'L', 'F', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      /* .e_type = */ ET_REL,
      /* .e_machine = */ EM_386,
      /* .e_version = */ 1,
      /* .e_entry = */ 0,
      /* .e_phoff = */ 0,
      /* .e_shoff = */ 0,  /* Will be changed later. */
      /* .e_flags = */ 0,
      /* .e_ehsize = */ sizeof(Elf32_Ehdr),  /* == 0x34. */
      /* .e_phentsize = */ 0,
      /* .e_phnum = */ 0,
      /* .e_shentsize = */ sizeof(Elf32_Shdr),  /* == 0x28. */
      /* .e_shnum = */ 0,  /* Will be changed later. */
      /* .e_shstrndx = */ 0,  /* Will be changed later. */
  };
  static Elf32_Shdr shdrs[SECTION_COUNT * 2 + 4], *shdr;
  static struct Section sections[SECTION_COUNT];
  struct Section *section, *section2;
  unsigned char rhd[4];  /* OMF record header. */
  unsigned rec_size, u;
  static unsigned char data[0x10000];  /* OMF record data. Buffer is large enough, because record size is 16 bits. Even the checksum byte fits. */
  unsigned char *up, *rdata, *rdata_end;
  char *lnames[0x20], *lname;
  unsigned lname_count;
  unsigned char seg_a, seg_combine, seg_big, seg_use32 = 0, seg_align, seg_idx, seg_count;
  unsigned seg_name_idx, seg_class_idx, seg_overlay_idx;
  uint32_t seg_size;
  unsigned grp_name_idx, grp_count, grp_dgroup, grp_flat, grp_idx;
  unsigned char fix_low, fix_location, fix_is_segrel, fix_f, fix_frame, fix_t, fix_p, fix_target;
  uint32_t fix_ofs, fix_disp, fix_fd, fix_td;
  uint32_t data_ofs;
  uint32_t elf_file_ofs;
  uint32_t lidata_repeat_count;
  unsigned lidata_block_count;
  unsigned char lidata_block_size;
  unsigned char *lidata_leoutp;
  unsigned rel_count;
  unsigned shdr_count;
  uint32_t allnames_size;
  char *allnames, *allnamep;
  unsigned extsymbol_count;
  struct Symbol *extsymbols, *extsymbolp, *extsymbols_end;
  unsigned pubsymbol_count;
  struct Symbol *pubsymbols, *pubsymbolp, *pubsymbols_end;
  struct Reloc *relocs, *relocp, *relocp_end;
  Elf32_Sym *syms, *symp, *syms_end;
  unsigned sym_count, local_sym_count;
  Elf32_Rel *rels, *relp, *rels_end;
  uint32_t rel_sym;
  uint32_t shdr_ofs;
  /* Mode 0 (don't optimize CONST, optimize YIB, this is the default): CONST is moved to the beginning of CONST2, YIB is moved to CONST and gets optimized.
   * Mode 1 (optimize CONST): CONST stays and gets optimized, CONST2 stays, a non-empty YIB is an error.
   * Mode 2 (autodetect): if -fpc was present and YIB is empty, then use mode 1, otherwise use mode 0.
   * Mode 3: Temporary mode for mode 2: -fpc was present, still detecting YIB.
   */
  char may_optimize_strings = 0;
  uint32_t orig_sdata_size, orig_yib_size;

  (void)argc; (void)argv;
#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__) || \
    defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__) || \
    defined(__powerpc__) || defined(_M_PPC) || defined(__m68k__)
#  error This program requires a little-endian system for ELF.  /* Otherwise we would have to do byte order conversion on Elf32_Ehdr etc. */
#endif
#if defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined (__MIPSEL) || defined(__MIPSEL__) || \
    defined(__ia64__) || IS_X86
  /* Known good little-endian system. */
#else
    if (*(unsigned char*)&ehdr.e_type != ET_REL) {  /* TODO(pts): Make this a compile-time check. */
      fprintf(stderr, "fatal: little endian system expected for ELF\n");
      return 2;
    }
#endif
  if (!argv[0] || !argv[1] || strcmp(argv[1], "--help") == 0) {
    fprintf(stderr, "Usage: %s [<flag>...] <omfobj.obj>\nFlags:\n"
            "-v: verbose operation, write info to stderr\n"
            "-h: strip one leading _ from non-.text symbol names\n"
            "-H: strip one trailing _ from symbol names\n"
            "-os0: never optimize strings\n"
            "-os1: always optimize strings\n"
            "-oss: optimize strings if it's safe\n"
            "-o <elfobj.o>: output ELF .o object filename\n",
            argv[0]);
    return !argv[0] || !argv[1];  /* 0 (EXIT_SUCCESS) for--help. */
  }
  for (argp = argv + 1; (arg = *argp) != NULL; ++argp) {
    if (arg[0] != '-') break;
    if (arg[1] == '\0') break;
    if (arg[1] == '-' && arg[2] == '\0') {
      ++argp;
      break;
    } else if (arg[1] == 'o' && arg[2] == 's' && arg[3] != '\0' && arg[4] == '\0') {
      if (arg[3] == '0') {
        may_optimize_strings = 0;
      } else if (arg[3] == '1') {
        may_optimize_strings = 1;
      } else if (arg[3] == 's') {
        may_optimize_strings = 2;
      } else {
        goto unknown_flag;
      }
    } else if (arg[2] != '\0') {
      goto unknown_flag;
    } else if (arg[1] == 'v') {
      ++verbose_level;
    } else if (arg[1] == 'h') {
      do_strip_one_leading_underscore = 1;
    } else if (arg[1] == 'H') {
      do_strip_one_trailing_underscore = 1;
    } else if (arg[1] == 'o' && argp[1]) {
      elfoname = *++argp;
    } else {
     unknown_flag:
      fprintf(stderr, "fatal: unknown command-line flag: %s\n", arg);
      return 1;
    }
  }
  if (!elfoname) {
    fprintf(stderr, "fatal: missing -o <elfobj.o> argument\n");
    return 1;
  }
  if (*argp == NULL) {
    fprintf(stderr, "fatal: missing filename argument\n");
    return 1;
  }
  omfname = *argp++;
  if (*argp != NULL) {
    fprintf(stderr, "fatal: too many commad-line arguments\n");
    return 1;
  }

  if ((f = fopen(omfname, "rb")) == NULL) {
    fprintf(stderr, "fatal: error opening <omfobj.obj>: %s\n", omfname);
    return 3;
  }
  is_pass2 = 0;
  lname_count = seg_count = grp_count = grp_flat = grp_dgroup = shdr_count = elf_file_ofs = extsymbol_count = pubsymbol_count = rel_count = sym_count = is_fpc = 0;
  orig_yib_size = orig_sdata_size = (uint32_t)-1;
  allnames_size = 1;
  allnames = allnamep = NULL;
  extsymbols_end = extsymbols = extsymbolp = NULL;
  pubsymbols_end = pubsymbols = pubsymbolp = NULL;
  relocs = relocp = NULL;
  syms = syms_end = symp = NULL;

 next_pass:
  unflushed_ledata_ofs = (uint32_t)-1;
  unflushed_ledata_section_idx = (unsigned char)-1;
  if (fread(rhd, 1, 4, f) != 4) {
    fprintf(stderr, "fatal: file too short for OMF: %s\n", omfname);
    return 4;
  }
  if (rhd[0] != THEADR || (rec_size = g16(rhd + 1)) != rhd[3] + 2U) {
    fprintf(stderr, "fatal: OMF signature not found: %s\n", omfname);
    return 5;
  }
  data[0] = rhd[3];
  if (fread(data + 1, 1, rec_size - 1, f) != rec_size - 1) {
    fprintf(stderr, "fatal: EOF in OMF record: %s\n", omfname);
    return 6;
  }
  if (data[rec_size - 1] != 0) {  /* Check the checksum. */
    rhd[3] = rhd[0] + rhd[1] + rhd[2];
    for (u = 0; u < rec_size; ++u) {
      rhd[3] += data[u];
    }
    if (rhd[3] != 0) {
      fprintf(stderr, "fatal: bad OMF record checksum: %s\n", omfname);
      return 7;
    }
  }
  for (;;) {  /* Read subsequent OMF records after the leading THEADR record. */
    if (fread(rhd, 1, 3, f) != 3) {
      fprintf(stderr, "fatal: EOF in OMF record: %s\n", omfname);
      return 8;
    }
    if (rhd[0] <= 0x80) {
      fprintf(stderr, "fatal: bad OMF record type: 0x%02x: %s\n", rhd[0], omfname);
      return 9;
    }
    rec_size = g16(rhd + 1);
    if (verbose_level > 1) fprintf(stderr, "info: found OMF record type=0x%02x size+1=0x%x\n", rhd[0], rec_size);
    if (rec_size == 0) {
      fprintf(stderr, "fatal: found OMF record of size 0: %s\n", omfname);
      return 10;
    }
    rdata = data;
    if (rhd[0] == LEDATA || rhd[0] == LEDATA386 || rhd[0] == LIDATA || rhd[0] == LIDATA386) {
      if (is_pass2 && (argc = flush_ledata(fdo, elfoname)) != 0) return argc;
      if (rhd[0] == LEDATA || rhd[0] == LEDATA386) rdata = unflushed_ledata_data;
    }
    up = rdata;
    if (fread(rdata, 1, rec_size, f) != rec_size) {
      fprintf(stderr, "fatal: EOF in OMF record: %s\n", omfname);
      return 11;
    }
    rdata_end = rdata + --rec_size;
    if (rdata[rec_size] != 0) {  /* Check the checksum. */
      rhd[3] = rhd[0] + rhd[1] + rhd[2];
      for (u = 0; u <= rec_size; ++u) {
        rhd[3] += rdata[u];
      }
      if (rhd[3] != 0) {
        fprintf(stderr, "fatal: bad OMF record checksum: %s\n", omfname);
        return 12;
      }
    }
    if (rhd[0] == MODEND || rhd[0] == MODEND386) {
      if (rec_size != 1 || data[0] != 0) {
        fprintf(stderr, "fatal: expected non-main module in OMF: %s\n", omfname);
        return 13;
      }
      break;  /* Explicit EOF indicator. */
    } else if (rhd[0] == LNAMES) {
      if (is_pass2) continue;
      while (up != rdata_end) {
        if (up + up[0] + 1 > rdata_end) {
          fprintf(stderr, "fatal: lname too long in OMF: %s\n", omfname);
          return 17;
        }
        if (lname_count == sizeof(lnames) / sizeof(lnames[0])) {
          fprintf(stderr, "fatal: too many lnames in OMF: %s\n", omfname);
          return 18;
        }
        lnames[lname_count++] = my_strdup_size((char*)up + 1, up[0]);
        up += up[0] + 1;
      }
    } else if (rhd[0] == SEGDEF || rhd[0] == SEGDEF386) {
      if (is_pass2) continue;
      if (seg_count++ >= 0x7f) {  /* Overly cautious, to make it fit in a signed char. */
        fprintf(stderr, "fatal: too many segments in OMF: %s\n", omfname);
        return 19;
      }
      if (up == rdata_end) {
       segdef_too_short:
        fprintf(stderr, "fatal: segdef too short in OMF: %s\n", omfname);
        return 20;
      }
      seg_a = *up++;
      seg_combine = (seg_a >> 2) & 7;
      seg_big = (seg_a >> 1) & 1;
      seg_use32 = seg_a & 1;
      seg_a >>= 5;
      seg_align = (seg_a == 1 ? 1 : seg_a == 2 ? 2 : seg_a == 3 ? 16 : seg_a == 5 ? 4 : 0);
      if (rhd[0] == SEGDEF) {
        if (up + 2 > rdata_end) goto segdef_too_short;
        seg_size = g16(up);
        up += 2;
        if (seg_big) {
          if (seg_size != 0) {
            fprintf(stderr, "fatal: bad big segment size in OMF: %s\n", omfname);
            return 21;
          }
          seg_size = 0x10000;
        }
      } else {  /* SEGDEF386 */
        if (up + 4 > rdata_end) goto segdef_too_short;
        seg_size = g32(up);
        up += 4;
        if (seg_big) {
          fprintf(stderr, "fatal: bad big segment in OMF: %s\n", omfname);
          return 22;
        }
      }
      if (verbose_level > 1) fprintf(stderr, "info: segdef a=%u combine=%u big=%u use32=%u align=%u size=0x%x\n", seg_a, seg_combine, seg_big, seg_use32, seg_align, seg_size);
      if (seg_align == 0) {
        fprintf(stderr, "fatal: bad segment alignment in OMF: a=%d: %s\n", seg_a, omfname);
        return 23;
      }
      if (seg_combine != 2 /* public */ && seg_combine != 0 /* private */) {
        fprintf(stderr, "fatal: bad segment combine in OMF: %d: %s\n", seg_combine, omfname);
        return 24;
      }
      if (up == rdata_end) goto segdef_too_short;
      seg_name_idx = *up++;
      if (seg_name_idx >= 0x80) {
        if (up == rdata_end) goto segdef_too_short;
        seg_name_idx = (seg_name_idx & 0x7f) << 8 | *up++;
      }
      seg_class_idx = *up++;
      if (seg_class_idx >= 0x80) {
        if (up == rdata_end) goto segdef_too_short;
        seg_class_idx = (seg_class_idx & 0x7f) << 8 | *up++;
      }
      seg_overlay_idx = *up++;
      if (seg_overlay_idx >= 0x80) {
        if (up == rdata_end) goto segdef_too_short;
        seg_overlay_idx = (seg_overlay_idx & 0x7f) << 8 | *up++;
      }
      if (up != rdata_end) {
        fprintf(stderr, "fatal: segdef too long in OMF: %s\n", omfname);
        return 25;
      }
      if (seg_name_idx-- == 0 || seg_name_idx >= lname_count) {
        fprintf(stderr, "fatal: bad name index in OMF: %s\n", omfname);
        return 26;
      }
      if (seg_class_idx-- == 0 || seg_class_idx >= lname_count) {
        fprintf(stderr, "fatal: bad class index in OMF: %s\n", omfname);
        return 27;
      }
      if (seg_overlay_idx-- == 0 || seg_overlay_idx >= lname_count) {
        fprintf(stderr, "fatal: bad overlay index in OMF: %s\n", omfname);
        return 28;
      }
      if (verbose_level > 1) fprintf(stderr, "info: segdef.. align=%u name=%d(%s) class=%d(%s) overlay=%d(%s) size=0x%x\n", seg_align, seg_name_idx, lnames[seg_name_idx], seg_class_idx, lnames[seg_class_idx], seg_overlay_idx, lnames[seg_overlay_idx], seg_size);
      lname = lnames[seg_name_idx];
      for (u = 0; u < SECTION_COUNT && strcmp(section_infos[u].seg_name, lname) != 0; ++u) {}
      if (u == SECTION_COUNT) {
        if (strcmp(lnames[seg_class_idx], "DWARF") == 0 && (memcmp(lname, ".debug_", 7) == 0 || memcmp(lname, ".WATCOM_", 8) == 0)) {
          /* Created by `owcc -gdwarf'. Ignore. */
        } else if (memcmp(lname, "$$", 2) == 0 && memcmp(lnames[seg_class_idx], "DEB", 3) == 0) {
          /* Created by `owcc -gcodeview' and `owcc -gwatcom'. Ignore. */
        } else {
          fprintf(stderr, "fatal: unknown segment in OMF: lname=%s class=%s: %s\n", lname, lnames[seg_class_idx], omfname);
          return 29;
        }
      } else {
        /***if (seg_use32 == 0) {
          fprintf(stderr, "fatal: expected 32-bit segment in OMF: %s\n", omfname);
          return 30;
        }***/
        if (seg_combine != 2 /* public */) {
          fprintf(stderr, "fatal: bad segment combine for normal segment in OMF: %s: %d: %s\n", lname, seg_combine, omfname);
          return 31;
        }
        if (strcmp(section_infos[u].class_name, lnames[seg_class_idx]) != 0) {
          fprintf(stderr, "fatal: bad segment class in OMF: name=%s got_class=%s expected_class=%s: %s\n", lname, lnames[seg_class_idx], section_infos[u].class_name, omfname);
          return 32;
        }
        if (lnames[seg_overlay_idx][0] != '\0') {
          fprintf(stderr, "fatal: bad segment overlay in OMF: name=%s overlay=%s: %s\n", lname, lnames[seg_overlay_idx], omfname);
          return 33;
        }
        section = sections + u;
        if (section->seg_idx != 0) {
          fprintf(stderr, "fatal: segment defined multiple times: %s: %s\n", lname, omfname);
          return 34;
        }
        section->seg_idx = seg_count;
        section->align = seg_align;
        section->size = seg_size;
        section->info = section_infos + u;
        section->rel_count = section->sym_count = 0;
      }
    } else if (rhd[0] == GRPDEF) {
      if (is_pass2) continue;
      if (grp_count++ >= 0x7f) {  /* Overly cautious. */
        fprintf(stderr, "fatal: too many groups in OMF: %s\n", omfname);
        return 40;
      }
      if (up == rdata_end) {
       grpdef_too_short:
        fprintf(stderr, "fatal: grpdef too short in OMF: %s\n", omfname);
        return 41;
      }
      grp_name_idx = *up++;
      if (grp_name_idx >= 0x80) {
        if (up == rdata_end) goto grpdef_too_short;
        grp_name_idx = (grp_name_idx & 0x7f) << 8 | *up++;
      }
      if (grp_name_idx-- == 0 || grp_name_idx >= lname_count) {
        fprintf(stderr, "fatal: bad group name index in OMF: %s\n", omfname);
        return 42;
      }
      lname = lnames[grp_name_idx];
      if (verbose_level > 1) fprintf(stderr, "info: grpdef name=%u(%s)\n", grp_name_idx, lname);
      if (strcmp(lname, "DGROUP") == 0 && grp_dgroup == 0) {
        grp_dgroup = grp_count;
      } else if (strcmp(lname, "FLAT") == 0 && grp_flat == 0) {
        grp_flat = grp_count;
      } else {
        fprintf(stderr, "fatal: bad group name in OMF: %s: %s\n", lname, omfname);
        return 43;
      }
      /* We just ignore the rest of grpdef. */
    } else if (rhd[0] == COMENT) {
      if (is_pass2) continue;
      /* There are many OMF comment types (i.e. metadata records). Just ignore most of them. */
      if (up + 6 <= rdata_end && up[0] == 0x80 && up[1] == 0x9b) {
        /* 88: rhd[0] OMF record type=COMENT
         * 0800: OMF record size=8
         * 80: COMENT bits=0x80 (CMT_TNP == no purge bit); `data' and `up' start here.
         * 9b: COMENT class=0x9b (ProcModel, CMT_WAT_PROC_MODEL, AT_PROC_MODEL, MODEL_COMMENT)
         * 33: processor='3' (i386)
         * 66: memory model='f' (FLAT)
         * 4f: optimized='O' (optimized). Always an 'O', hardcoded.
         * 70: up[5] floating point='p' (inline, -fpi87, but -fpi also does it); 'e' would be emulated-inline (-fpi...?); 'c' would be emulated (-fpc)
         * 64: Always a 'd', hardcoded.
         *
         * Generated by wcc386 in OutModel(...) in https://github.com/open-watcom/open-watcom-v2/blob/6366191465a4414a2633e982607e776a1950eab2/bld/cg/intel/c/x86obj.c#L884-L910
         */
        if (may_optimize_strings == 2) {
          may_optimize_strings = (is_fpc = up[5] == 'c') ? 3 : 0;
        }
      }
    } else if (rhd[0] == LINNUM386) {
      /* Created by `owcc -gwatcom'. Ignore. */
    } else if (rhd[0] == LEDATA || rhd[0] == LEDATA386 || rhd[0] == LIDATA || rhd[0] == LIDATA386) {
      if (up == rdata_end) {
       ledata_too_short:
        fprintf(stderr, "fatal: ledata too short in OMF: %s\n", omfname);
        return 35;
      }
      seg_idx = *up++;
      if (seg_idx >= 0x80) {  /* This never happens in practice, the OMF files have just 5 segments (see in section_infos). */
        fprintf(stderr, "fatal: seg_idx too large in ledata in OMF: %s\n", omfname);
        return 36;
      }
      if (seg_idx == 0 || seg_idx > seg_count) {
        fprintf(stderr, "fatal: bad ledata segment index in OMF: %s\n", omfname);
        return 36;
      }
      for (u = 0, section = sections; u < SECTION_COUNT && section->seg_idx != seg_idx; ++u, ++section) {}
      if (u == SECTION_COUNT) continue;  /* This happens for debug segments etc. */
      if (rhd[0] == LEDATA || rhd[0] == LIDATA) {
        if (up + 2 > rdata_end) goto ledata_too_short;
        data_ofs = g16(up);
        up += 2;
      } else {  /* LEDATA386 or LIDATA386. */
        if (up + 4 > rdata_end) goto ledata_too_short;
        data_ofs = g32(up);
        up += 4;
      }
      if (is_pass2) {
        if (orig_sdata_size != (uint32_t)-1) {
          if (u == SECTION_SDATA) {
            ++u; ++section;  /* Put it to the beginning of SECTION_RODATA instead. */
          } else if (u == SECTION_RODATA) {
            data_ofs += orig_sdata_size;
          }
        }
        if (orig_yib_size != (uint32_t)-1) {
          if (u == SEG_YIB) {
            u = SECTION_SDATA; section = sections + SECTION_SDATA;
          }
        }
        if (section->shndx == 0) {
          fprintf(stderr, "fatal: assert: ledata in ghost segment in OMF: %s: %s\n", section->info->seg_name, omfname);
          return 79;
        }
      }
      unflushed_ledata_section_file_ofs = section->file_ofs;
      unflushed_ledata_ofs = data_ofs;  /* Affects the next fixupp* record. */
      unflushed_ledata_section_idx = u;
      if (rhd[0] == LIDATA || rhd[0] == LIDATA386) {
        lidata_leoutp = unflushed_ledata_data;
        while (up != rdata_end) {
          if (up + (rhd[0] == LIDATA ? 4 : 6) > rdata_end) {
           lidata_too_short:
            fprintf(stderr, "fatal: lidata too short in OMF: %s\n", omfname);
            return 77;
          }
          if (rhd[0] == LIDATA) {
            lidata_repeat_count = g16(up);
            up += 2;
          } else {  /* LIDATA386. */
            lidata_repeat_count = g32(up);
            up += 4;
          }
          lidata_block_count = g16(up);
          up += 2;
          if (lidata_block_count != 0) {  /* This is valid OMF, but we don't support it, because OpenWatcom doesn't emit it. */
            if (verbose_level > 1) fprintf(stderr, "info: lidata repeat_count=0x%x block_count=0x%x\n", (unsigned)lidata_repeat_count, (unsigned)lidata_block_count);
            fprintf(stderr, "fatal: nonzero lidata block count not allowed in OMF: %s\n", omfname);
            return 77;
          }
          if (up == rdata_end) goto lidata_too_short;
          lidata_block_size = *up++;
          if (up + lidata_block_size > rdata_end) goto lidata_too_short;
          if (lidata_repeat_count != 0 && lidata_block_size != 0) {
            if (u == SECTION_BSS) goto data_in_bss;
            /* The maximum OpenWatcom emits: lidata_repeat_count <= 0x2000, lidata_block_size == 1. We allow a bit more (sizeof(unflushed_ledata_data) >= 0x1000). */
            if (verbose_level > 1) fprintf(stderr, "info: lidata repeat_count=0x%x block_size=0x%x\n", (unsigned)lidata_repeat_count, lidata_block_size);
            if (lidata_repeat_count > 0xffffff || (size_t)(unflushed_ledata_data + sizeof(unflushed_ledata_data) - lidata_leoutp) < lidata_repeat_count * lidata_block_size) {
              fprintf(stderr, "fatal: lidata data too long in OMF: %s\n", omfname);
              return 77;
            }
            if (lidata_block_size == 1) {  /* Shortcut. */
              memset(lidata_leoutp, *up, lidata_repeat_count);
            } else {
              do {
                memcpy(lidata_leoutp, up, lidata_block_size);
                lidata_leoutp += lidata_block_size;
              } while (--lidata_repeat_count > 0);
            }
          }
          up += lidata_block_size;
        }
        unflushed_ledata_size = lidata_leoutp - unflushed_ledata_data;
        unflushed_ledata_up = unflushed_ledata_data;
        /* Flush early, so that a subsequent FIXUPP record can't reference
         * this LIDATA. It is not needed by OpenWatcom, and it would be
         * complicated to implement.
         */
        if ((argc = flush_ledata(fdo, elfoname)) != 0) return argc;
      } else {
        seg_size = rdata_end - up;
        if (verbose_level > 1) fprintf(stderr, "info: ledata segment=%s data_ofs=0x%x data_size=0x%x\n", section->info->seg_name, data_ofs, seg_size);
        if (u == SECTION_BSS && seg_size != 0) {
         data_in_bss:
          fprintf(stderr, "fatal: trying to write data to _BSS segment in OMF: %s\n", omfname);
          return 37;
        }
        unflushed_ledata_size = seg_size;
        unflushed_ledata_up = up;
        /* Don't flush yet, a subsequent FIXUPP record needs this data unflushed. */
      }
      if (data_ofs + unflushed_ledata_size > section->size) {
        fprintf(stderr, "fatal: found data beyond segment size: segment=%s data_end=0x%x segment_size=0x%x: %s\n", section->info->seg_name, data_ofs + unflushed_ledata_size, section->size, omfname);
        return 15;
      }
    } else if (rhd[0] == EXTDEF || rhd[0] == LEXTDEF) {
      while (up != rdata_end) {
        if (up + up[0] + 2 > rdata_end) {
          fprintf(stderr, "fatal: name too long in OMF: %s\n", omfname);
          return 44;
        }
        if (is_pass2) {
          if (allnamep + up[0] + 1 > allnames + allnames_size) {
            fprintf(stderr, "fatal: assert: name too long\n");
            return 45;
          }
          if (extsymbolp == extsymbols_end) {
            fprintf(stderr, "fatal: assert: too many ext symbols\n");
            return 45;
          }
          if (memchr((char*)up + 1, '\0', up[0])) {
            fprintf(stderr, "fatal: NUL in name in OMF: %s\n", omfname);
            return 46;
          }
          extsymbolp->is_global = (rhd[0] == EXTDEF);
          extsymbolp->prev_idx = (uint32_t)-1;
          extsymbolp->value = 0;  /* Placeholder. */
          extsymbolp->name = allnamep;
          extsymbolp->section = SYMBOL_SECTION_EXTERN;
          memcpy(allnamep, up + 1, up[0]);
          allnamep[up[0]] = '\0';
          if (do_strip_one_leading_underscore && allnamep[0] == '_' && !is_openwatcom_libc_symbol(allnamep)) ++extsymbolp->name;
          allnamep += up[0] + 1;
          if (do_strip_one_trailing_underscore) {
            if (up[0] > 1 && allnamep[-2] == '_') {
              allnamep[-2] = '\0';
            }
          }
          ++extsymbolp;
        } else {
          if (extsymbol_count >= 0x3fffffff / sizeof(struct Symbol)) {  /* To avoid overflows. */
            fprintf(stderr, "fatal: too many ext symbols in OMF: %s\n", omfname);
            return 47;
          }
          ++extsymbol_count;
          allnames_size += up[0] + 1;
        }
        up += up[0] + 1;
        if (up[0] != 0) {
          fprintf(stderr, "fatal: bad type for name in OMF: %s\n", omfname);
          return 48;
        }
        ++up;
      }
    } else if (rhd[0] == PUBDEF || rhd[0] == LPUBDEF || rhd[0] == PUBDEF386 || rhd[0] == LPUBDEF386) {
      if (up == rdata_end) {
       pubdef_too_short:
        fprintf(stderr, "fatal: pubdef too short in OMF: %s\n", omfname);
        return 49;
      }
      grp_idx = *up++;
      if (grp_idx >= 0x80) {
        if (up == rdata_end) goto pubdef_too_short;
        grp_idx = (grp_idx & 0x7f) << 8 | *up++;
      }
      if (grp_idx > grp_count || (grp_idx != grp_flat && grp_idx != grp_dgroup)) {
        fprintf(stderr, "fatal: unknown group in pubdef in OMF: %s\n", omfname);
        return 50;
      }
      if (up == rdata_end) goto pubdef_too_short;
      seg_idx = *up++;
      if (seg_idx >= 0x80) {
        if (up == rdata_end) goto pubdef_too_short;
        seg_idx = (seg_idx & 0x7f) << 8 | *up++;
      }
      if (seg_idx > seg_count) {
        fprintf(stderr, "fatal: bad segment name index in pubdef in OMF: %s\n", omfname);
        return 51;
      }
      for (u = 0, section = sections; u < SECTION_COUNT && section->seg_idx != seg_idx; ++u, ++section) {}
      if (u == SECTION_COUNT) {
        fprintf(stderr, "fatal: unknown segment in pubdef in OMF: %s\n", omfname);
        return 52;
      }
      if ((u == SECTION_SDATA && (!is_fpc || (rhd[0] == PUBDEF || rhd[0] == PUBDEF386))) || u == SEG_YIB) {
        fprintf(stderr, "fatal: segment %s not allowed in pubdef in OMF: %s\n", section->info->seg_name, omfname);
        return 52;
      }
      section = sections + u;
      if ((grp_idx == grp_flat) != (u == SECTION_TEXT)) {
        fprintf(stderr, "fatal: group mismatch in pubdef in OMF: grp=%s seg=%s: %s\n", grp_idx == grp_flat ? "FLAT" : "DGROUP", section->info->seg_name, omfname);
        return 54;
      }
      if (verbose_level > 2) fprintf(stderr, "info: pubdef seg=%s\n", section->info->seg_name);
      seg_use32 = (rhd[0] == PUBDEF386 || rhd[0] == LPUBDEF386);
      while (up != rdata_end) {
        if (up + up[0] + 2 + (seg_use32 ? 4 : 2) > rdata_end) {
          fprintf(stderr, "fatal: name too long in pubdef in OMF: %s\n", omfname);
          return 55;
        }
        if (is_pass2) {
          if (allnamep + up[0] + 1 > allnames + allnames_size) {
            fprintf(stderr, "fatal: assert: name too long\n");
            return 56;
          }
          if (pubsymbolp == pubsymbols_end) {
            fprintf(stderr, "fatal: assert: too many pub symbols\n");
            return 57;
          }
          if (memchr((char*)up + 1, '\0', up[0])) {
            fprintf(stderr, "fatal: NUL in name in OMF: %s\n", omfname);
            return 58;
          }
          pubsymbolp->is_global = (rhd[0] == PUBDEF || rhd[0] == PUBDEF386);
          pubsymbolp->prev_idx = pubsymbolp - pubsymbols;
          pubsymbolp->name = allnamep;
          pubsymbolp->section = u;
          memcpy(allnamep, up + 1, up[0]);
          allnamep[up[0]] = '\0';
          if (do_strip_one_leading_underscore && allnamep[0] == '_' && !is_openwatcom_libc_symbol(allnamep)) ++pubsymbolp->name;
          allnamep += up[0] + 1;
          if (do_strip_one_trailing_underscore) {
            if (up[0] > 1 && allnamep[-2] == '_') {
              allnamep[-2] = '\0';
            }
          }
        } else {
          if (pubsymbol_count >= 0x3fffffff / sizeof(struct Symbol)) {  /* To avoid overflows. */
            fprintf(stderr, "fatal: too many pub symbols in OMF: %s\n", omfname);
            return 59;
          }
          ++pubsymbol_count;
          allnames_size += up[0] + 1;
          ++section->sym_count;
        }
        up += up[0] + 1;
        if (is_pass2) {
          pubsymbolp->value = seg_use32 ? g32(up) : g16(up);
          if (orig_sdata_size != (uint32_t)-1) {
            if (u == SECTION_SDATA) {
              ++pubsymbolp->section; ++u; ++section;  /* Put it to SECTION_RODATA instead. */
            } else if (u == SECTION_RODATA) {
              pubsymbolp->value += orig_sdata_size;
            }
          }
          if (orig_yib_size != (uint32_t)-1) {
            if (u == SEG_YIB) {
              pubsymbolp->section = u = SECTION_SDATA; section = sections + SECTION_SDATA;
            }
          }
          if (section->shndx == 0) {
            fprintf(stderr, "fatal: assert: pubdef in ghost segment in OMF: %s: %s\n", section->info->seg_name, omfname);
            return 75;
          }
          if (verbose_level > 1) fprintf(stderr, "info: pubdef name=%s seg=%s value=0x%x is_global=%d\n", pubsymbolp->name, section->info->seg_name, (unsigned)pubsymbolp->value, pubsymbolp->is_global);
          ++pubsymbolp;
        }
        up += (seg_use32 ? 4 : 2);
        if (up[0] != 0) {
          fprintf(stderr, "fatal: bad type for name in OMF: %s\n", omfname);
          return 60;
        }
        ++up;
      }
    } else if (rhd[0] == FIXUPP || rhd[0] == FIXUPP386) {
      seg_use32 = (rhd[0] == FIXUPP386);
      if (unflushed_ledata_section_idx + 1 == 0 || unflushed_ledata_ofs + 1 == 0) {
        fprintf(stderr, "fatal: unknown segment in fixupp in OMF: %s\n", omfname);  /* This can happen after LIDATA, but in practice OpenWatcom doesn't generate suce a file. */
        return 61;
      }
      if (is_pass2 && section->shndx == 0) {
        fprintf(stderr, "fatal: assert: fixupp in ghost segment in OMF: %s: %s\n", section->info->seg_name, omfname);
        return 76;
      }
      while (up != rdata_end) {
        fix_low = *up++;
        if ((fix_low & 0x80) == 0) {
          fprintf(stderr, "fatal: found thread subrecord in fixupp in OMF: %s\n", omfname);
          return 62;
        }
        if (up + 3 > rdata_end) {
         fixupp_subrecord_too_long:
          fprintf(stderr, "fatal: subrecord too long in fixupp in OMF: %s\n", omfname);
          return 63;
        }
        fix_ofs = *up++ | (fix_low & 3) << 8;
        if (fix_ofs + (seg_use32 ? 4 : 2) > unflushed_ledata_size) {
          fprintf(stderr, "fatal: fixupp offset beyond end of segment in OMF: limit=0x%x: %s\n", unflushed_ledata_size, omfname);
          return 64;
        }
        fix_ofs += unflushed_ledata_ofs;
        fix_is_segrel = (fix_low >> 6) & 1;
        fix_location = (fix_low >> 2) & 0xf;
        if (1 - 0 && verbose_level > 1) fprintf(stderr, "info: fixupp is_segrel=%d ofs=0x%x loc=%d\n", fix_is_segrel, fix_ofs, fix_location);
        if (fix_location != 9 && fix_location != 1) {  /* 32- or 16-bit offset. */
          fprintf(stderr, "info: bad fixupp location type in OMF: %d: %s\n", fix_location, omfname);
          return 65;
        }
        fix_target = *up++;
        fix_f = fix_target >> 7;
        fix_frame = (fix_target >> 4) & 0xf;
        fix_t = (fix_target >> 3) & 1;
        fix_p = (fix_target >> 2) & 1;
        fix_target &= 3;
        if (1 - 0 && verbose_level > 1) fprintf(stderr, "info: fixupp is_segrel=%d ofs=0x%x f=%d frame=%d t=%d p=%d target=%d\n", fix_is_segrel, fix_ofs, fix_f, fix_frame, fix_t, fix_p, fix_target);
        if (fix_f != 0) {
          fprintf(stderr, "info: bad fixupp f in OMF: %s\n", omfname);
          return 66;
        }
        if (fix_t != 0) {
          fprintf(stderr, "info: bad fixupp t in OMF: %s\n", omfname);
          return 67;
        }
        if (fix_frame != 1 && fix_frame != 5) {  /* Only F0, F1 and F2 need a frame datum. F1(GRPDEF) does. Add F5 for 16-bit */
          fprintf(stderr, "info: bad fixupp frame in OMF: %d: %s\n", fix_frame, omfname);
          return 68;
        }
        if (fix_target != 0 && fix_target != 2) {  /* Which T? have a frame datum? T0(SEGDEF) and T2(EXTDEF) do. */
          fprintf(stderr, "info: bad fixupp target in OMF: %d: %s\n", fix_target, omfname);
          return 69;
        }
        if (up == rdata_end) goto fixupp_subrecord_too_long;
        if (fix_frame == 1) {
          fix_fd = *up++;
          if (fix_fd >= 0x80) {
            if (up == rdata_end) goto fixupp_subrecord_too_long;
            fix_fd = (fix_fd & 0x7f) << 8 | *up++;
          }
        } else fix_fd = grp_dgroup;
        if (up == rdata_end) goto fixupp_subrecord_too_long;
        fix_td = *up++;
        if (fix_td >= 0x80) {
          if (up == rdata_end) goto fixupp_subrecord_too_long;
          fix_td = (fix_td & 0x7f) << 8 | *up++;
        }
        if (fix_p) {
          fix_disp = 0;
        } else {
          if (up + (seg_use32 ? 4 : 2) > rdata_end) goto fixupp_subrecord_too_long;
          fix_disp = (seg_use32 ? g32(up) : g16(up));
          up += (seg_use32 ? 4 : 2);
        }
        if (fix_disp != 0) {
          fprintf(stderr, "info: bad fixupp disp in OMF: %u: %s\n", (unsigned)fix_disp, omfname);
          return 70;
        }
        if (fix_target != 0) {
          if (fix_td == 0 || fix_td > extsymbol_count) {
            fprintf(stderr, "info: bad fixupp target ext idx in OMF: %u: %s\n", fix_td, omfname);
            return 71;
          }
          u = fix_td - 1;
        } else if (fix_target == 0) {
          if (fix_td == 0 || fix_td > seg_count) {
            fprintf(stderr, "info: bad fixupp target seg idx in OMF: %u: %s\n", fix_td, omfname);
            return 72;
          }
          for (u = 0; u < SECTION_COUNT && sections[u].seg_idx != fix_td; ++u) {}
          if (u == SECTION_COUNT) {
            fprintf(stderr, "info: assert bad fixupp target seg value in OMF: %u: %s\n", fix_td, omfname);
            return 73;
          }
          if (orig_sdata_size != (uint32_t)-1) {
            if (u == SECTION_SDATA) {
              ++u;
            } else if (u == SECTION_RODATA) {
              if (seg_use32) {
                if (verbose_level > 1) fprintf(stderr, "info: ADD32 %d,%d\n", fix_ofs, orig_sdata_size);
                add32(unflushed_ledata_up + fix_ofs - unflushed_ledata_ofs, orig_sdata_size);
              } else {
                if (verbose_level > 1) fprintf(stderr, "info: ADD16 %d,%d\n", fix_ofs, orig_sdata_size);
                add16(unflushed_ledata_up + fix_ofs - unflushed_ledata_ofs, orig_sdata_size);
              }
            }
          }
          if (is_pass2 && orig_yib_size != (uint32_t)-1) {
            if (u == SEG_YIB) {
              u = SECTION_SDATA;
            }
          }
          sections[u].is_rel_target = 1;
        }
        if (rel_count >= 0x3fffffff / sizeof(struct Reloc)) {  /* To avoid overflows. */
          fprintf(stderr, "fatal: too many relocations in OMF: %s\n", omfname);
          /* !! remove(elfofn); on failure */
          return 74;
        }
        /* `section' is used from the previous LEDATA. */
        if (is_pass2) {
          if (section->relocp == section->relocs + section->rel_count) {
            fprintf(stderr, "fatal: assert: bad section relocation count in OMF: %s\n", omfname);
          }
          section->relocp->ofs = fix_ofs;
          section->relocp->idx = u;
          /*section->relocp->section_idx = unflushed_ledata_section_idx;*/
          section->relocp->is_segrel = fix_is_segrel;
          section->relocp->is_extdef = fix_target != 0;
          if (!fix_is_segrel) {  /* OMF and ELF have a different idea about PC-based relocation. We fix it for R_386_PC32 relocations by subtracting 4 from each value. */
            if (seg_use32) {
              if (verbose_level > 1) fprintf(stderr, "info: ADD32 %d,-4\n", fix_ofs);
              add32(unflushed_ledata_up + fix_ofs - unflushed_ledata_ofs, -4);
            } else {
              if (verbose_level > 1) fprintf(stderr, "info: ADD16 %d,-2\n", fix_ofs);
              add16(unflushed_ledata_up + fix_ofs - unflushed_ledata_ofs, -2);
            }
          }
          ++section->relocp;
        } else {
          ++rel_count;
          ++section->rel_count;
        }
        /* OpenWatcom emits fix_fd == grp_flat; NASM emits fix_fd == grp_dgroup. It doesn't matter. */
        if (verbose_level > 1) fprintf(stderr, "info: fixupp is_segrel=%d ofs=0x%x seg=%s frame_group=%s target=%s td=%d disp=%d\n", fix_is_segrel, fix_ofs, sections[unflushed_ledata_section_idx].info->seg_name, fix_fd == grp_flat ? "FLAT" : "DGROUP", fix_target == 0 ? "SEGDEF" : "EXTDEF", fix_td, fix_disp);
      }
    } else {
      fprintf(stderr, "fatal: unsupported OMF record type 0x%02x: %s\n", rhd[0], omfname);
      return 14;
    }
  }

  if (!is_pass2) {  /* Transition from pass 1 to pass 2. */
    if (0 && verbose_level > 1) {
      for (u = 0; u < lname_count; ++u) {
        fprintf(stderr, "info: lnames[%u] = (%s)\n", u, lnames[u]);
      }
    }
    if (verbose_level > 1) fprintf(stderr, "info: entering pass 2\n");
    if (may_optimize_strings == 2) {  /* No -fpc. */
      may_optimize_strings = 0;
      if (verbose_level) fprintf(stderr, "info: may not optimize strings in CONST (-fpi)\n");
    } else if (may_optimize_strings == 3) {  /* Had -fpc. */
      if (sections[SEG_YIB].size != 0 || sections[SEG_YIB].rel_count != 0 || sections[SEG_YIB].sym_count != 0 || sections[SEG_YIB].is_rel_target) {
        may_optimize_strings = 0;
        if (verbose_level) fprintf(stderr, "info: may not optimize strings in CONST (-fpc and YIB)\n");
      } else {
        may_optimize_strings = 1;
        if (verbose_level) fprintf(stderr, "info: may optimize strings in CONST (-fpc and no YIB)\n");
      }
    } else {
      if (sections[SEG_YIB].size != 0 || sections[SEG_YIB].rel_count != 0 || sections[SEG_YIB].sym_count != 0 || sections[SEG_YIB].is_rel_target) {
        if (verbose_level) fprintf(stderr, "info: found strings to optimize in YIB\n");
      }
    }
    if (may_optimize_strings) sections[SECTION_SDATA].align = 1;  /* Override align=4 of segment CONST (as emitted by wcc386) with align=1. */
    unflushed_ledata_size = 0;
    is_pass2 = 1;
    elf_file_ofs = sizeof(Elf32_Ehdr);
    allnamep = allnames = my_xmalloc(allnames_size);
    *allnamep++ = '\0';  /* To let symbol counting start from 1. */
    extsymbols = (struct Symbol*)my_xmalloc(sizeof(struct Symbol) * extsymbol_count);  /* TODO(pts): Check for overflow. */
    extsymbols_end = extsymbols + extsymbol_count;
    pubsymbols = (struct Symbol*)my_xmalloc(sizeof(struct Symbol) * pubsymbol_count);  /* TODO(pts): Check for overflow. */
    pubsymbols_end = pubsymbols + pubsymbol_count;
    relocp = relocs = (struct Reloc*)my_xmalloc(sizeof(struct Reloc) * rel_count);
    shdr_count = 1;  /* NULL. */
    sym_count = 1;  /* NULL. */
    for (u = 0, section = sections; u < SECTION_COUNT; ++u, ++section) {
      if (section->info)
      if (verbose_level > 1) fprintf(stderr, "info: after pass1 segment name=%s section=%s size=0x%x rel_count=%u sym_count=%u is_rel_target=%u\n", section->info->seg_name, section->info->rel_section_name + 4, section->size, section->rel_count, section->sym_count, section->is_rel_target);
      if (u == SECTION_SDATA && !may_optimize_strings &&
          (section->size != 0 || section->rel_count != 0 || section->sym_count != 0 || section->is_rel_target)) {
        /* Put everything to SECTION_RODATA (section[1]) instead of
         * SECTION_SDATA (section[0], *section), so that GNU ld(1) won't try
         * to misinterpret and optimize generic data as ASCIIZ strings.
         */
        orig_sdata_size = section->size;  /* Can be zero. */
        section2 = sections + SECTION_RODATA;  /* == section + 1. */
        if (section2->size != 0) orig_sdata_size += -orig_sdata_size & (section2->align - 1);  /* Add alignment for SECTION_RODATA after SECTION_SDATA. */
        if (section2->align < section->align) section2->align = section->align;  /* No-op, section->align == 1. */
        section2->size += orig_sdata_size;
        section2->rel_count += section->rel_count;
        section2->sym_count += section->sym_count;
        if (section->is_rel_target) section2->is_rel_target = 1;
        section->size = section->rel_count = section->sym_count = section->is_rel_target = 0;
      }
      if (u == SECTION_SDATA && !may_optimize_strings) {
        section->size = orig_yib_size = sections[SEG_YIB].size;
        section->rel_count = sections[SEG_YIB].rel_count;
        section->sym_count = sections[SEG_YIB].sym_count;
        section->is_rel_target = sections[SEG_YIB].is_rel_target;
        section->align = sections[SEG_YIB].align = 1;  /* Override align=4 of segment CONST (as emitted by wcc386) with align=1. */
        sections[SEG_YIB].size = sections[SEG_YIB].rel_count = sections[SEG_YIB].sym_count = sections[SEG_YIB].is_rel_target = 0;
      }
      if (!(section->size != 0 || section->rel_count != 0 || section->sym_count != 0 || section->is_rel_target)) continue;
      if (u >= SEG_YIB) continue;  /* Keep them as ghost segments, intentionally. pass 2 will fail for them. */
      section->shndx = shdr_count++;
      elf_file_ofs = (elf_file_ofs + 3) & ~3;  /* Round up to a multiple of 4, like how GNU as(1) does. */
      section->file_ofs = elf_file_ofs;
      elf_file_ofs += section->size;
      if (section->rel_count != 0) {
        section->relocp = section->relocs = relocp;
        relocp += section->rel_count;
        elf_file_ofs = (elf_file_ofs + 3) & ~3;  /* Round up to a multiple of 4, like how GNU as(1) does. */
        section->rel_file_ofs = elf_file_ofs;
        elf_file_ofs += sizeof(Elf32_Rel) * section->rel_count;
        ++shdr_count;  /* The .rel.* shdr */
      }
      if (section->is_rel_target) {
        section->symndx = sym_count++;
      }
    }
    shdr_count += 3;  /* .shstrtab, .strtab, .symtab. */
    if (relocp != relocs + rel_count) {
      fprintf(stderr, "fatal: assert: bad total relocation count in OMF: %s\n", omfname);
      return 90;
    }
    sym_count += pubsymbol_count + extsymbol_count;  /* This will eventually be smaller because of duplication. */
    elf_file_ofs = (elf_file_ofs + 3) & ~3;  /* Round up to a multiple of 4. */
    ehdr.e_shoff = elf_file_ofs;
    ehdr.e_shnum = shdr_count;
    ehdr.e_shstrndx = shdr_count - 3;  /* .shstrtab. */
    /* Other fields of ehdr are correct, ready to write. */
    symp = syms = (Elf32_Sym*)my_xmalloc(sizeof(Elf32_Sym) * sym_count);
    memset(syms, '\0', sizeof(Elf32_Sym) * sym_count);
    ++symp;  /* Skip the NULL section. */
    for (u = 0, section = sections; u < SECTION_COUNT; ++u, ++section) {
      if (section->is_rel_target) {
        symp->st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
        symp->st_shndx = section->shndx;
        ++symp;
      }
    }
    /* Rewind f (OMF input) and write ehdr to fdo (ELF .o). */
    if (fseek(f, 0, SEEK_SET)) {
      fprintf(stderr, "fatal: error seeking to beginning in OMF: %s\n", omfname);
      return 91;
    }
    if ((fdo = open(elfoname, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {  /* O_RDWR for reading relocations. */
      fprintf(stderr, "fatal: error opening ELF .o for writing: %s\n", elfoname);
      return 92;
    }
    if ((size_t)write(fdo, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
      fprintf(stderr, "fatal: error writing ELF .o ehdr: %s\n", elfoname);
      return 93;
    }
    extsymbolp = extsymbols;
    pubsymbolp = pubsymbols;
    goto next_pass;
  }

  /* Here we are after pass 2. */
  if ((argc = flush_ledata(fdo, elfoname)) != 0) return argc;
  if (allnamep != allnames + allnames_size) {
    fprintf(stderr, "fatal: assert: bad total name size in OMF: %s\n", omfname);
    return 94;
  }
  for (extsymbolp = extsymbols; extsymbolp != extsymbols_end; ++extsymbolp) {
    lname = (char*)extsymbolp->name;
    for (pubsymbolp = pubsymbols, u = 0; pubsymbolp != pubsymbols_end; ++pubsymbolp, ++u) {
      if (strcmp(pubsymbolp->name, lname) == 0) {
        --sym_count;  /* It was counted twice: once as pubsymbol, once as extsymbol. */
        extsymbolp->prev_idx = u;
        if (!extsymbolp->is_global && extsymbolp->prev_idx == (uint32_t)-1) {
          fprintf(stderr, "info: local extern extsymbol not allowed in OMF: %s: %s\n", extsymbolp->name, omfname);
          return 95;
        }
        if (extsymbolp->is_global != pubsymbolp->is_global) {
          fprintf(stderr, "info: bad localness of duplicate extsymbol: %s: %s\n", extsymbolp->name, omfname);
          return 96;
        }
        break;
      }
    }
  }
  syms_end = syms + sym_count;  /* sym_count has just got its final value. */
  u = symp - syms;
  for (pubsymbolp = pubsymbols; pubsymbolp != pubsymbols_end; ++pubsymbolp) {
    if (!pubsymbolp->is_global) {
      if (verbose_level > 1) fprintf(stderr, "info: found %s #%u (%s)\n", "local", u, pubsymbolp->name);
      if (symp == syms_end) {
       bad_elf_sym_count:
        fprintf(stderr, "fatal: assert: bad total symbol count in OMF: %s\n", omfname);
        return 97;
      }
      symp->st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE);
      symp->st_shndx = sections[pubsymbolp->section].shndx;
      symp->st_name = pubsymbolp->name - allnames;
      symp->st_value = pubsymbolp->value;
      pubsymbolp->prev_idx = u++;
      ++symp;
    }
  }
  local_sym_count = u;
  for (pubsymbolp = pubsymbols; pubsymbolp != pubsymbols_end; ++pubsymbolp) {
    if (pubsymbolp->is_global) {
      if (verbose_level > 1) fprintf(stderr, "info: found %s #%u (%s)\n", "global", u, pubsymbolp->name);
      symp->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
      symp->st_shndx = sections[pubsymbolp->section].shndx;
      symp->st_name = pubsymbolp->name - allnames;
      symp->st_value = pubsymbolp->value;
      pubsymbolp->prev_idx = u++;
      ++symp;
    }
  }
  for (extsymbolp = extsymbols; extsymbolp != extsymbols_end; ++extsymbolp) {
    if (extsymbolp->prev_idx == (uint32_t)-1) {
      if (verbose_level > 1) fprintf(stderr, "info: found extern #%u (%s)\n", u, extsymbolp->name);
      symp->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
      symp->st_shndx = SHN_UNDEF;
      symp->st_name = extsymbolp->name - allnames;
      extsymbolp->prev_idx = u++;
      ++symp;
    } else {
      extsymbolp->prev_idx = pubsymbols[extsymbolp->prev_idx].prev_idx;
    }
  }
  if (symp != syms_end) goto bad_elf_sym_count;
  /* First create and write the ELF shdr. */
  shdr_ofs = elf_file_ofs;
  up = data;  /* Create .shstrtab. */
  *up++ = '\0';  /* Start with an empty name. */
  for (u = 0, section = sections, shdr = shdrs + 1; u < SECTION_COUNT; ++u, ++section) {
    if (!(section->size != 0 || section->rel_count != 0 || section->sym_count != 0 || section->is_rel_target)) continue;
    if (section->shndx == 0) {
      fprintf(stderr, "fatal: assert: found ghost section in OMF: %s: %s\n", section->info->rel_section_name + 4, omfname);
      return 78;
    }
    shdr->sh_type = (u == SECTION_BSS) ? SHT_NOBITS : SHT_PROGBITS;
    shdr->sh_offset = section->file_ofs;
    shdr->sh_size = section->size;
    shdr->sh_addralign = section->align;
    shdr->sh_entsize = (u == SECTION_SDATA) ? 1 : 0;
    shdr->sh_flags = section->info->sh_flags;
    if (section->rel_count != 0) {  /* Has a .rel. shdr. */
      strcpy((char*)up, section->info->rel_section_name);
      shdr->sh_name = up - data + 4;  /* strlen(".rel") == 4. */
      ++shdr;
      shdr->sh_name = up - data;  /* Starting with `.rel'. */
      up += strlen((char*)up) + 1;
      shdr->sh_type = SHT_REL;
      shdr->sh_link = shdr_count - 2;  /* .symtab. */
      shdr->sh_addralign = 4;
      shdr->sh_info = (shdr - 1) - shdrs;  /* Points to the coressponding non-.rel. shdr. */
      shdr->sh_entsize = sizeof(Elf32_Rel);
      shdr->sh_offset = section->rel_file_ofs;
      shdr->sh_size = sizeof(Elf32_Rel) * section->rel_count;
      ++shdr;
      relocp_end = section->relocs + section->rel_count;
      if (section->relocp != relocp_end) {
        fprintf(stderr, "fatal: assert: bad relocp in OMF: %s\n", omfname);
        return 99;
      }
      if (verbose_level > 1) {
        for (relocp = section->relocs; relocp != relocp_end; ++relocp) {
          if (relocp->is_extdef) {
            fprintf(stderr, "info: reloc in section=%s ofs=0x%x is_segrel=%d add_sym=(%s)\n", section->info->rel_section_name + 4, (unsigned)relocp->ofs, relocp->is_segrel, extsymbols[relocp->idx].name);
          } else {
            fprintf(stderr, "info: reloc in section=%s ofs=0x%x is_segrel=%d add_section=%s\n", section->info->rel_section_name + 4, (unsigned)relocp->ofs, relocp->is_segrel, sections[relocp->idx].info->rel_section_name + 4);
          }
        }
      }
    } else {
      strcpy((char*)up, section->info->rel_section_name + 4);
      shdr->sh_name = up - data;
      up += strlen((char*)up) + 1;
      ++shdr;
    }
  }
  if ((size_t)(shdr - shdrs) != shdr_count - 3) {
    fprintf(stderr, "fatal: assert: bad shdr count in ELF .o: %s\n", elfoname);
    return 80;
  }
  elf_file_ofs += (char*)(shdr + 3) - (char*)shdrs;  /* Skip the ELF shdrs. */
  /* Now create .shstrtab. */
  shdr->sh_type = SHT_STRTAB;
  shdr->sh_addralign = 1;
  strcpy((char*)up, ".shstrtab");
  shdr->sh_name = up - data;
  up += strlen((char*)up) + 1;
  ++shdr;
  /* Now create .symtab. */
  shdr->sh_type = SHT_SYMTAB;
  shdr->sh_entsize = sizeof(Elf32_Sym);
  shdr->sh_size = sizeof(Elf32_Sym) * sym_count;
  shdr->sh_link = shdr_count - 1;  /* .strtab. */
  shdr->sh_addralign = 4;
  /* In each symbol table, all symbols with STB_LOCAL binding precede the
   * weak and global symbols. A symbol table section's sh_info section
   * header member holds the symbol table index for the first non-local
   * symbol.
   */
  shdr->sh_info = local_sym_count;
  strcpy((char*)up, ".symtab");
  shdr->sh_name = up - data;
  up += strlen((char*)up) + 1;
  ++shdr;
  /* Now create .strtab. */
  shdr->sh_type = SHT_STRTAB;
  shdr->sh_size = allnames_size;  /* TODO(pts): Deduplicate names between extsymbols and pubsymbols. */
  shdr->sh_addralign = 1;
  strcpy((char*)up, ".strtab");
  shdr->sh_name = up - data;
  up += strlen((char*)up) + 1;
  ++shdr;
  /* Now fix the .sh_offset and .sh_size fields. */
  elf_file_ofs = (elf_file_ofs + 3) & ~3;  /* Round up to a multiple of 4. */
  shdr[-2].sh_offset = elf_file_ofs;  /* .symtab. */
  elf_file_ofs += shdr[-2].sh_size;
  shdr[-1].sh_offset = elf_file_ofs;  /* .strtab. */
  elf_file_ofs += shdr[-1].sh_size;
  elf_file_ofs = (elf_file_ofs + 3) & ~3;  /* Round up to a multiple of 4. */
  shdr[-3].sh_offset = elf_file_ofs;  /* .shstrtab. */
  shdr[-3].sh_size = up - data;  /* .shstrtab. */
  seg_size = (char*)shdr - (char*)shdrs;
  if ((uint32_t)lseek(fdo, shdr_ofs, SEEK_SET) != shdr_ofs) {  /* Usually this succeeds, and then the write fails. */
    fprintf(stderr, "fatal: error seeking in ELF .o to shdr: %s\n", elfoname);
    return 98;
  }
  if ((size_t)write(fdo, shdrs, seg_size) != seg_size) {
    fprintf(stderr, "fatal: error writing shdr to ELF .o: %s\n", elfoname);
    return 81;
  }

  /* Now create and write .rel.* sections. */
  seg_size = 0;
  for (u = 0, section = sections; u < SECTION_COUNT; ++u, ++section) {
    if (section->rel_count > seg_size) seg_size = section->rel_count;
  }
  rels = (Elf32_Rel*)my_xmalloc(sizeof(Elf32_Rel) * seg_size);
  for (u = 0, section = sections; u < SECTION_COUNT; ++u, ++section) {
    if (section->rel_count == 0) continue;
    for (relp = rels, rels_end = rels + section->rel_count, relocp = section->relocs; relp != rels_end; ++relp, ++relocp) {
      rel_sym = relocp->is_extdef ? extsymbols[relocp->idx].prev_idx : sections[relocp->idx].symndx;
      relp->r_offset = relocp->ofs;
      if (seg_use32)
        relp->r_info = ELF32_R_INFO(rel_sym, relocp->is_segrel ? R_386_32 : R_386_PC32);
      else
        relp->r_info = ELF32_R_INFO(rel_sym, relocp->is_segrel ? R_386_16: R_386_PC16);
    }
    elf_file_ofs = section->rel_file_ofs;  /* .rel.* */
    if ((uint32_t)lseek(fdo, elf_file_ofs, SEEK_SET) != elf_file_ofs) {  /* Usually this succeeds, and then the write fails. */
      fprintf(stderr, "fatal: error seeking in ELF .o to %s: %s\n", sections->info->rel_section_name, elfoname);
      return 82;
    }
    seg_size = (char*)rels_end - (char*)rels;
    if ((size_t)write(fdo, rels, seg_size) != seg_size) {
      fprintf(stderr, "fatal: error writing %s to ELF .o: %s\n", sections->info->rel_section_name, elfoname);
      return 83;
    }
  }

  elf_file_ofs = shdr[-2].sh_offset;  /* .symtab. */
  if ((uint32_t)lseek(fdo, elf_file_ofs, SEEK_SET) != elf_file_ofs) {  /* Usually this succeeds, and then the write fails. */
    fprintf(stderr, "fatal: error seeking in ELF .o to .symtab: %s\n", elfoname);
    return 84;
  }
  seg_size = shdr[-2].sh_size;
  if ((size_t)write(fdo, syms, seg_size) != seg_size) {
    fprintf(stderr, "fatal: error writing .symtab to ELF .o: %s\n", elfoname);
    return 85;
  }
  elf_file_ofs = shdr[-1].sh_offset;  /* .strtab. */
  if ((uint32_t)lseek(fdo, elf_file_ofs, SEEK_SET) != elf_file_ofs) {  /* Usually this succeeds, and then the write fails. */
    fprintf(stderr, "fatal: error seeking in ELF .o to .strtab: %s\n", elfoname);
    return 86;
  }
  seg_size = shdr[-1].sh_size;
  if ((size_t)write(fdo, allnames, seg_size) != seg_size) {
    fprintf(stderr, "fatal: error writing .strtab to ELF .o: %s\n", elfoname);
    return 87;
  }
  elf_file_ofs = shdr[-3].sh_offset;  /* .shstrtab. */
  if ((uint32_t)lseek(fdo, elf_file_ofs, SEEK_SET) != elf_file_ofs) {  /* Usually this succeeds, and then the write fails. */
    fprintf(stderr, "fatal: error seeking in ELF .o to .shstrtab: %s\n", elfoname);
    return 88;
  }
  seg_size = shdr[-3].sh_size;
  if ((size_t)write(fdo, data, seg_size) != seg_size) {
    fprintf(stderr, "fatal: error writing .shstrtab to ELF .o: %s\n", elfoname);
    return 89;
  }

  if (verbose_level > 1) fprintf(stderr, "info: successfully written ELF.o: %s\n", elfoname);
  /*fclose(f);*/  /* No need, we exit anyway. */
  /*close(fdo);*/  /* No need, we exit anyway. */
  return 0;
}
