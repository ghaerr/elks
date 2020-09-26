/*
 * Convert an ELF "executable" file which employs H. Peter Anvin's segelf
 * relocations, into an ELKS executable.
 * Copyright (c) 2020 TK Chia
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; see the file COPYING3.LIB.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef __DJGPP__
# include <libgen.h>
#endif
#include "libelf.h"

struct minix_exec_hdr
{
  uint32_t type;
  uint8_t hlen;
  uint8_t reserved1;
  uint16_t version;
  uint32_t tseg;
  uint32_t dseg;
  uint32_t bseg;
  uint32_t entry;
  uint16_t chmem;
  uint16_t minstack;
  uint32_t syms;
};

struct elks_supl_hdr_1
{
  uint32_t trsize;
  uint32_t drsize;
  uint32_t tbase;
  uint32_t dbase;
};

struct elks_supl_hdr_2
{
  uint32_t ftseg;
  uint32_t ftrsize;
  uint32_t reserved2, reserved3;
};

struct minix_reloc
{
  uint32_t vaddr;
  uint16_t symndx;
  uint16_t type;
};

#define MINIX_COMBID	((uint32_t) 0x04100301ul)
#define MINIX_SPLITID_AHISTORICAL ((uint32_t) 0x04300301ul)

#define R_SEGWORD	80

#define S_TEXT		((uint16_t) -2u)
#define S_DATA		((uint16_t) -3u)
#define S_BSS		((uint16_t) -4u)
#define S_FTEXT		((uint16_t) -5u)

static const char *me;
static bool verbose = false, tiny = false;
static const char *file_name = NULL;
static char *tmp_file_name = NULL;
static uint16_t total_data = 0, chmem = 0, stack = 0, heap = 0, entry = 0;
static int ifd = -1, ofd = -1;
static Elf *elf = NULL;
static Elf_Scn *text = NULL, *ftext = NULL, *data = NULL, *bss = NULL,
	       *rel_dyn = NULL;
static const Elf32_Shdr *text_sh = NULL, *ftext_sh = NULL, *data_sh = NULL,
			*bss_sh = NULL, *rel_dyn_sh = NULL;
static uint32_t text_n_rels = 0, ftext_n_rels = 0, data_n_rels = 0;

static void
error_exit (void)
{
  if (elf)
    elf_end (elf);
  if (ifd != -1)
    close (ifd);
  if (ofd != -1)
    close (ofd);
  if (tmp_file_name)
    unlink (tmp_file_name);
  exit (1);
}

static void
error_1 (const char *fmt, va_list ap)
{
  fprintf (stderr, "%s: error: ", me);
  vfprintf (stderr, fmt, ap);
}

static void
error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  error_1 (fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
  error_exit ();
}

static void
error_with_errno (const char *fmt, ...)
{
  int err = errno;
  va_list ap;
  va_start (ap, fmt);
  error_1 (fmt, ap);
  va_end (ap);
  fprintf (stderr, ": %s\n", strerror (err));
  error_exit ();
}

static void
error_with_elf_msg (const char *fmt, ...)
{
  int err = elf_errno ();
  va_list ap;
  va_start (ap, fmt);
  error_1 (fmt, ap);
  va_end (ap);
  fprintf (stderr, ": %s\n", elf_errmsg (err));
  error_exit ();
}

static void
info (const char *fmt, ...)
{
  fprintf (stderr, "%s: ", me);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
}

#define INFO(fmt, ...)	do \
			  { \
			    if (verbose) \
			      info ((fmt), __VA_ARGS__); \
			  } \
			while (0)

static void
parm_uint16 (int argc, char **argv, uint16_t *pvalue, int *pi)
{
  char *ep;
  int i = *pi + 1;
  *pi = i;
  if (i >= argc)
    error ("expected integer argument after `%s'", argv[i - 1]);

  uintmax_t x = strtoumax(argv[i], &ep, 0);
  if (x > 0xffffu || *ep != 0)
    error ("invalid integer argument `%s'", argv[i]);

  *pvalue = x;
}

static void
parse_args (int argc, char **argv)
{
  int i = 1;

  while (i < argc)
    {
      const char *arg = argv[i];

      if (arg[0] == '-')
	{
	  if (strcmp (arg + 1, "v") == 0)
	    verbose = true;
	  else if (arg[1] == '-')
	    {
	      if (strcmp (arg + 2, "tiny") == 0)
		tiny = true;
	      else if (strcmp(arg + 2, "total-data") == 0)
		parm_uint16 (argc, argv, &total_data, &i);
	      else if (strcmp(arg + 2, "chmem") == 0)
		parm_uint16(argc, argv, &chmem, &i);
	      else if (strcmp(arg + 2, "stack") == 0)
		parm_uint16(argc, argv, &stack, &i);
	      else if (strcmp(arg + 2, "heap") == 0)
		parm_uint16(argc, argv, &heap, &i);
	      else
		error ("unknown option `%s'", arg);
	    }
	  else
	    error ("unknown option `%s'", arg);
	}
      else if (file_name)
	error ("multiple file names!");
      else
	file_name = arg;

      ++i;
    }

  if (!file_name)
    error ("no file specified!");

  if (total_data)
    {
      if (chmem)
	error ("cannot specify both --total-data and --chmem");
      if (stack)
	error ("cannot specify both --total-data and --stack");
      if (heap)
	error ("cannot specify both --total-data and --heap");
    }

  if (chmem)
    {
      if (stack)
	error ("cannot specify both --chmem and --stack");
      if (heap)
	error ("cannot specify both --chmem and --heap");
    }
}

static void
set_scn (Elf_Scn **pscn, const Elf32_Shdr **psh, Elf_Scn *scn,
	 const Elf32_Shdr *shdr, const char *nature, size_t sidx)
{
  if (*pscn)
    error ("cannot have more than one %s section!", nature);

  INFO ("ELF section %#zx -> %s section", sidx, nature);
  INFO ("\tvirt. addr. %#" PRIx32 ", size %#" PRIx32
	", file offset %#" PRIx32,
	shdr->sh_addr, shdr->sh_size, shdr->sh_offset);

  if ((uint32_t) (shdr->sh_addr + shdr->sh_size) < shdr->sh_addr)
    error ("malformed %s section: segment bounds wrap around!");

  switch (shdr->sh_type)
    {
    case SHT_PROGBITS:
    case SHT_NOBITS:
      if (shdr->sh_size > (uint32_t) 0xffffu)
	error ("%s section is too large (%#" PRIx32 " > 0xffff)",
	       shdr->sh_size);
      break;
    default:
      ;
    }

  *pscn = scn;
  *psh = shdr;
}

static bool
in_scn_p (uint32_t addr, const Elf32_Shdr *shdr)
{
  return shdr && addr >= shdr->sh_addr && addr < shdr->sh_addr + shdr->sh_size;
}

static void
check_scn_overlap (const Elf32_Shdr *shdr1, const char *nature1,
		   const Elf32_Shdr *shdr2, const char *nature2)
{
  if (! shdr1 || ! shdr2)
    return;

  if (in_scn_p (shdr1->sh_addr, shdr2)
      || in_scn_p (shdr2->sh_addr, shdr1))
    error ("%s and %s sections overlap!", nature1, nature2);
}

static void
input_for_header (void)
{
  size_t num_scns, sidx;
  Elf32_Ehdr *ehdr;

  ifd = open (file_name, O_RDONLY);
  if (ifd == -1)
    error_with_errno ("cannot open input file `%s'", file_name);

  elf = elf_begin (ifd, ELF_C_READ, NULL);
  if (! elf)
    error_with_elf_msg ("cannot open input file `%s' as ELF", file_name);

  ehdr = elf32_getehdr (elf);
  if (! ehdr)
    error_with_elf_msg ("cannot get ELF header");

  if (ehdr->e_machine != EM_386)
    error ("`%s' is not an x86 ELF file");

  if (elf_getshdrnum (elf, &num_scns) != 0)
    error_with_elf_msg ("cannot get ELF section count");

  if (num_scns < 2)
    error_with_elf_msg ("ELF input has no sections");

  for (sidx = 1; sidx < num_scns; ++sidx)
    {
      Elf_Scn *scn;
      Elf32_Shdr *shdr;
      const char *name;

      scn = elf_getscn (elf, sidx);
      if (! scn)
	error_with_elf_msg ("cannot read ELF section %#zx", sidx);

      shdr = elf32_getshdr (scn);
      if (! shdr)
	error_with_elf_msg ("cannot read ELF section %#zx header", sidx);

      switch (shdr->sh_type)
	{
	case SHT_REL:
	  if (shdr->sh_info == 0)
	    set_scn (&rel_dyn, &rel_dyn_sh, scn, shdr, "dynamic relocations",
		     sidx);
	  /* TODO: also make use of section-specific relocation sections if
	     they are present. */

	  break;

	case SHT_PROGBITS:
	  name = elf_strptr (elf, ehdr->e_shstrndx, shdr->sh_name);
	  if (! name)
	    error_with_elf_msg ("cannot read ELF section %#zx name", sidx);

	  if (strcmp (name, ".text") == 0)
	    set_scn (&text, &text_sh, scn, shdr, "text", sidx);
	  else if (strcmp (name, ".fartext") == 0)
	    set_scn (&ftext, &ftext_sh, scn, shdr, "far text", sidx);
	  else if (strcmp (name, ".data") == 0)
	    set_scn (&data, &data_sh, scn, shdr, "data", sidx);
	  else if (shdr->sh_size != 0 && (shdr->sh_flags & SHF_ALLOC) != 0)
	    error ("stray SHT_PROGBITS SHF_ALLOC section %#zx `%s'", sidx,
								     name);

	  break;

	case SHT_NOBITS:
	  name = elf_strptr (elf, ehdr->e_shstrndx, shdr->sh_name);
	  if (! name)
	    error_with_elf_msg ("cannot read ELF section %#zx name", sidx);

	  if (strcmp (name, ".bss") == 0)
	    set_scn (&bss, &bss_sh, scn, shdr, "BSS", sidx);
	  else if (shdr->sh_size != 0 && (shdr->sh_flags & SHF_ALLOC) != 0)
	    error ("stray SHT_NOBITS SHF_ALLOC section %#zx `%s'", sidx, name);

	  break;

	default:
	  ;  /* ignore other types of sections */
	}
    }

  if (! in_scn_p (ehdr->e_entry, text_sh))
    error ("entry point outside near text segment");

  entry = ehdr->e_entry - text_sh->sh_addr;

  check_scn_overlap (text_sh, "text", ftext_sh, "far text");
  check_scn_overlap (text_sh, "text", data_sh, "data");
  check_scn_overlap (text_sh, "text", bss_sh, "BSS");
  check_scn_overlap (ftext_sh, "far text", data_sh, "data");
  check_scn_overlap (ftext_sh, "far text", bss_sh, "BSS");
  check_scn_overlap (data_sh, "data", bss_sh, "BSS");

  if (tiny)
    {
      if (ftext)
	error ("tiny model program cannot have far text segment!");

      if (text)
	{
	  if (data
	      && text_sh->sh_addr + text_sh->sh_size != data_sh->sh_addr)
	    error ("data segment must come right after text in tiny model!");

	  if (! data && bss
	      && text_sh->sh_addr + text_sh->sh_size != bss_sh->sh_addr)
	    error ("data segment must come right after text in tiny model!");
	}
    }

  if (rel_dyn)
    {
      Elf_Data *stuff = elf_getdata (rel_dyn, NULL);
      size_t stuff_size;
      const Elf32_Rel *prel;

      if (! stuff)
	error_with_elf_msg ("cannot read dynamic relocations");

      stuff_size = stuff->d_size;
      if (stuff_size != rel_dyn_sh->sh_size)
	error ("short ELF read of dynamic relocations");

      if (! stuff_size || stuff_size % sizeof (Elf32_Rel) != 0)
	error ("weirdness when reading dynamic relocations!");

      prel = (const Elf32_Rel *) stuff->d_buf;
      while (stuff_size)
	{
	  uint32_t vaddr = prel->r_offset;

	  if (in_scn_p (vaddr, text_sh))
	    {
	      ++text_n_rels;
	      if (text_n_rels > (uint32_t) 0x8000u)
		error ("too many text segment relocations");
	    }
	  else if (in_scn_p (vaddr, ftext_sh))
	    {
	      ++ftext_n_rels;
	      if (ftext_n_rels > (uint32_t) 0x8000u)
		error ("too many far text segment relocations");
	    }
	  else if (in_scn_p (vaddr, data_sh))
	    {
	      ++data_n_rels;
	      if (data_n_rels > (uint32_t) 0x8000u)
		error ("too many data segment relocations");
	    }
	  else
	    error ("stray relocation outside text and data sections!");

	  ++prel;
	  stuff_size -= sizeof (Elf32_Rel);
	}
    }

  INFO ("%" PRIu32 " text reloc(s)., %" PRIu32 " far text reloc(s)., "
	"%" PRIu32 " data reloc(s).", text_n_rels, ftext_n_rels, data_n_rels);
}

static void
start_output (void)
{
  char *dir;
  size_t dir_len;

  tmp_file_name = malloc (strlen (file_name) + 8);
  if (! tmp_file_name)
    error_with_errno ("gut reaction %d", (int) __LINE__);

  strcpy (tmp_file_name, file_name);

  dir = dirname (tmp_file_name);
  /* Argh... */
  if (dir != tmp_file_name)
    strcpy (tmp_file_name, dir);

  dir_len = strlen (tmp_file_name);
  if (dir_len && tmp_file_name[dir_len - 1] != '/'
	      && tmp_file_name[dir_len - 1] != '\\')
    {
      tmp_file_name[dir_len] = '/';
      ++dir_len;
    }
  strcpy (tmp_file_name + dir_len, "XXXXXX");

  ofd = mkstemp (tmp_file_name);
  if (ofd == -1)
    error_with_errno ("cannot create temporary output file");

  INFO ("created temporary file `%s'", tmp_file_name);
}

static void
output (const void *buf, size_t n)
{
  ssize_t r;
  const char *p = (const char *) buf;
  while (n)
    {
      r = write (ofd, p, n);

      if (r < 0)
	error_with_errno ("cannot write output file");

      if (! r)
	error ("cannot write output file: disk full?");

      p += r;
      n -= r;
    }
}

static void
output_header (void)
{
  struct minix_exec_hdr mh;
  struct elks_supl_hdr_1 esuph1;
  struct elks_supl_hdr_2 esuph2;

  memset (&mh, 0, sizeof mh);
  memset (&esuph1, 0, sizeof esuph1);
  memset (&esuph2, 0, sizeof esuph2);

  if (tiny)
    mh.type = MINIX_COMBID;
  else
    mh.type = MINIX_SPLITID_AHISTORICAL;

  if (text_sh)
    mh.tseg = text_sh->sh_size;
  if (bss_sh)
    mh.bseg = bss_sh->sh_size;
  if (data_sh)
    {
      mh.dseg = data_sh->sh_size;
      mh.bseg += bss_sh->sh_addr - data_sh->sh_addr - data_sh->sh_size;
    }
  mh.entry = entry;

  if (total_data)
    {
      mh.version = 0;
      mh.chmem = total_data;
      mh.minstack = 0;
    }
  else if (chmem)
    {
      uint32_t accum = 0;
      mh.version = 0;
      if (tiny)
	accum = mh.tseg + mh.dseg + mh.bseg;
      else
	accum = mh.dseg + mh.bseg;
      if (accum + chmem > 0xfff0)
	error ("data segment too big with --chmem!");
      mh.chmem = accum + chmem;
      mh.minstack = 0;
    }
  else if (! heap && ! stack)
    {
      mh.version = tiny ? 0 : 1;
      mh.chmem = mh.minstack = 0;
    }
  else
    {
      mh.version = 1;
      mh.chmem = heap;
      mh.minstack = stack;
    }

  start_output ();

  if ((ftext_sh && ftext_sh->sh_size) || ftext_n_rels)
    {
      mh.hlen = sizeof mh + sizeof esuph1 + sizeof esuph2;
      esuph1.trsize = (uint32_t) text_n_rels * sizeof (struct minix_reloc);
      esuph2.ftseg = ftext_sh->sh_size;
      esuph2.ftrsize = (uint32_t) ftext_n_rels * sizeof (struct minix_reloc);
      output (&mh, sizeof mh);
      output (&esuph1, sizeof esuph1);
      output (&esuph2, sizeof esuph2);
    }
  else if (text_n_rels || data_n_rels)
    {
      mh.hlen = sizeof mh + sizeof esuph1;
      esuph1.trsize = (uint32_t) text_n_rels * sizeof (struct minix_reloc);
      output (&mh, sizeof mh);
      output (&esuph1, sizeof esuph1);
    }
  else
    {
      mh.hlen = sizeof mh;
      output (&mh, sizeof mh);
    }
}

static void
output_scn_stuff (Elf_Scn *scn, const Elf32_Shdr *shdr, const char *nature)
{
  Elf_Data *stuff;
  size_t stuff_size;

  if (! scn)
    return;

  stuff = elf_getdata (scn, NULL);
  if (! stuff)
    error_with_elf_msg ("cannot read %s segment contents", nature);

  stuff_size = stuff->d_size;
  if (stuff_size != shdr->sh_size)
    error ("short ELF read of %s segment", nature);

  output (stuff->d_buf, stuff_size);
}

static void
output_scns_stuff (void)
{
  output_scn_stuff (text, text_sh, "text");
  output_scn_stuff (ftext, ftext_sh, "far text");
  output_scn_stuff (data, data_sh, "data");
}

#define R_386_SEGRELATIVE 48

static void
convert_reloc (struct minix_reloc *porel, const Elf32_Rel *prel,
	       Elf_Scn *place_scn, uint16_t offset_in_scn)
{
  Elf_Data *stuff;
  uint8_t r_type = ELF32_R_TYPE (prel->r_info);
  const uint8_t *buf;
  uint16_t addend;

  if (r_type != R_386_SEGRELATIVE)
    error ("unknown ELF relocation type %u", (unsigned) r_type);

  stuff = elf_getdata (place_scn, NULL);
  if (! stuff)
    error ("gut reaction %d", (int) __LINE__);

  buf = (const uint8_t *) stuff->d_buf;
  addend = (uint16_t) buf[offset_in_scn + 1] << 8 | buf[offset_in_scn];

  if (addend * 0x10 == text_sh->sh_addr)
    porel->symndx = S_TEXT;
  else if (addend * 0x10 == ftext_sh->sh_addr)
    porel->symndx = S_FTEXT;
  else if (addend * 0x10 == data_sh->sh_addr)
    porel->symndx = S_DATA;
  else
    {
      const char *place_scn_name = "<unknown>";
      if (place_scn == text)
	place_scn_name = ".text";
      else if (place_scn == ftext)
	place_scn_name = ".fartext";
      else if (place_scn == data)
	place_scn_name = ".data";
      error ("R_386_SEGRELATIVE relocation at place %s:%#" PRIx16 " has bad "
	     "in-place addend %#" PRIx16,
	     place_scn_name, offset_in_scn, addend);
    }

  porel->vaddr = offset_in_scn;
  porel->type = R_SEGWORD;
}

static void
output_relocs (void)
{
  uint32_t tot_n_rels = text_n_rels + ftext_n_rels + data_n_rels;
  struct minix_reloc *orels;
  uint32_t tridx = 0, ftridx = text_n_rels, dridx = text_n_rels + ftext_n_rels;
  Elf_Data *stuff;
  size_t stuff_size;
  const Elf32_Rel *prel;

  if (! tot_n_rels)
    return;

  orels = malloc (tot_n_rels * sizeof (struct minix_reloc));
  if (! orels)
    error_with_errno ("cannot create output relocations");

  stuff = elf_getdata (rel_dyn, NULL);
  if (! stuff)
    error_with_elf_msg ("cannot read dynamic relocations");

  stuff_size = stuff->d_size;
  if (stuff_size != rel_dyn_sh->sh_size)
    error ("short ELF read of dynamic relocations");
  if (! stuff_size || stuff_size % sizeof (Elf32_Rel) != 0)
    error ("weirdness when reading dynamic relocations!");

  prel = (const Elf32_Rel *) stuff->d_buf;
  while (stuff_size)
    {
      uint32_t vaddr = prel->r_offset;

      if (in_scn_p (vaddr, text_sh))
	{
	  if (tridx >= tot_n_rels)
	    error ("gut reaction %d", (int) __LINE__);
	  convert_reloc (&orels[tridx], prel, text, vaddr - text_sh->sh_addr);
	  ++tridx;
	}
      else if (in_scn_p (vaddr, ftext_sh))
	{
	  if (ftridx >= tot_n_rels)
	    error ("gut reaction %d", (int) __LINE__);
	  convert_reloc (&orels[ftridx], prel, ftext,
			 vaddr - ftext_sh->sh_addr);
	  ++ftridx;
	}
      else if (in_scn_p (vaddr, data_sh))
	{
	  if (dridx >= tot_n_rels)
	    error ("gut reaction %d", (int) __LINE__);
	  convert_reloc (&orels[dridx], prel, data, vaddr - data_sh->sh_addr);
	  ++dridx;
	}
      else
	error ("stray relocation outside text and data sections!");

      ++prel;
      stuff_size -= sizeof (Elf32_Rel);
    }

  output (orels, tot_n_rels * sizeof (struct minix_reloc));
  free (orels);
}

static void
end_output (void)
{
  struct stat orig_stat;
  int res;

  close (ofd);
  ofd = -1;

  if (stat (file_name, &orig_stat) == 0)
    chmod (tmp_file_name, orig_stat.st_mode & ~(mode_t) S_IFMT);

  res = rename (tmp_file_name, file_name);

  if (res != 0 && errno == EEXIST)
    {
      unlink (file_name);
      res = rename (tmp_file_name, file_name);
    }

  if (res != 0)
    error ("cannot rename `%s' to `%s'", tmp_file_name, file_name);
}

int
main(int argc, char **argv)
{
  me = argv[0];
  parse_args (argc, argv);
  elf_version (1);
  input_for_header ();
  output_header ();
  output_scns_stuff ();
  output_relocs ();
  end_output ();
  return 0;
}
