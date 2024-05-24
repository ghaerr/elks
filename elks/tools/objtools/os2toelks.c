/*
 * prog16cv: converter for 16-bit x86 executables formats
 * by pts@fazekas.hu at Thu Jan 19 20:41:19 CET 2023
 *
 * Right now the input must be an OS/2 1.x .exe program (output of `owcc
 * -bos2'), and the output must be an ELKS executable program.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__WATCOMC__) && defined(__LINUX__)
#undef O_BINARY  /* Work around OpenWatcom libc bug. */
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#ifdef __WATCOMC__
#define NORETURN __declspec(aborts)
#endif
#endif

NORETURN static void fail0(const char *msg) {
  (void)!write(2, msg, strlen(msg));
  (void)!write(2, "\n", 1);
  exit(1);
}

NORETURN static void fail1(const char *msg, const char *arg) {
  (void)!write(2, msg, strlen(msg));
  fail0(arg);
}

/* TODO(pts): Shortcut for x86. */
static uint8_t get_u8_le(const char *p) {
  return ((unsigned char*)p)[0];
}

/* TODO(pts): Shortcut for x86. */
static uint16_t get_u16_le(const char *p) {
  return ((unsigned char*)p)[0] | ((unsigned char*)p)[1] << 8;
}

/* TODO(pts): Shortcut for x86. */
static uint32_t get_u32_le(const char *p) {
  return ((unsigned char*)p)[0] | ((unsigned char*)p)[1] << 8 | (uint32_t)((unsigned char*)p)[2] << 16 | (uint32_t)((unsigned char*)p)[3] << 24;
}

static void set_u16_le(char *p, uint16_t v) {
  *p++ = v;
  v >>= 8;
  *p = v;
}

static void set_u32_le(char *p, uint32_t v) {
  *p++ = v;
  v >>= 8;
  *p++ = v;
  v >>= 8;
  set_u16_le(p, v);
}

static char buf[0x1000];

static int copy_fd(int infd, int outfd, uint32_t size) {
  int got;
  for (; size > 0; size -= got) {
    got = size > sizeof(buf) ? sizeof(buf) : size;
    if (read(infd, buf, got) != got) return 1;
    if (write(outfd, buf, got) != got) return 2;
  }
  return 0;  /* Success. */
}

#define HDR_SIZE 0x200  /* <= sizeof(buf). */
typedef hdr_size_assert[HDR_SIZE <= sizeof(buf)];
#define hdr buf  /* Reuse array. */

/* NE header in the input file:
0 .Signature	db 'NE'
2 .LinkerVer	db 5
3 .LinkerRev	db 0
4 .EntryTabOfs	dw EntryTab-ne_header
6 .EntryTabSize	dw EntryTab.end-EntryTab
8 .ChkSum		dd 0
12 .ProgramFlags	db PFLAG.INST
13 .ApplicationFlags db 0
14 .AutoDataSegNo	dw 1
16 .HeapInitSize	dw 0
18 .StackSize	dw STACK_SIZE  ; Do we need larger?
20 .InitIp 	dw _start-segment_code
22 .InitCsSegNo	dw 2
24 .InitSp:	dw segment_data_end-segment_data+STACK_SIZE
26 .InitSsSegNo	dw 1
28 .SegCnt 	dw (SegTab.end-SegTab)>>3
30 .ModCnt		dw (ModRefTab.end-ModRefTab)>>1
32 .NonResNameTabSize dw ..@NonResNameTab.end-..@NonResNameTab
34 .SegTabOfs	dw SegTab-ne_header
36 .ResourceTabOfs	dw ResourceTab-ne_header
38 .ResNameTabOfs	dw ResNameTab-ne_header
40 .ModRefTabOfs	dw ModRefTab-ne_header
42 .ImpNameTabOfs	dw ImpNameTab-ne_header
44 .NonResNameTabOfs dd ..@NonResNameTab-$$
48 .MovableEntryCnt dw 0
50 .SegAlignShift	dw FILE_ALIGNMENT_SHIFT
52 .ResourceSegCnt	dw 0
54 .OsFlags	db OSFLAG.UNKNOWN
55 .ExeFlags	db 0
56 .FastLoadOfs	dw 0
58 .FastLoadSize	dw 0
60 .Reserved	dw 0  ; Minimum code swap area size (?).
62 .ExpectedWinVer	dw 0
64
*/
#define GET_NE_SAS(ne) get_u16_le((ne) + 50)
#define GET_NE_STACK_SIZE(ne) get_u16_le((ne) + 18)
#define GET_NE_INIT_IP(ne) get_u16_le((ne) + 20)
#define GET_NE_SEG_TAB_OFS(ne) get_u16_le((ne) + 34)
#define GET_NE_AUTO_DATA_SEG_NO(ne) get_u16_le((ne) + 14)
#define GET_NE_INIT_CS_SEG_NO(ne) get_u16_le((ne) + 22)
#define GET_NE_INIT_SS_SEG_NO(ne) get_u16_le((ne) + 26)

/* NE segment table entry in the input file:
0 .fofs      dw (segment_data-$$)>>FILE_ALIGNMENT_SHIFT
2 .size	     dw segment_data_end+2-segment_data ; Must not be 0 for OS/2 1.0.
4 .flags     dw $SEG.DPL3 | $SEG.DATA
6 .minalloc  dw segment_data_end-segment_data+STACK_SIZE
8
*/

int main(int argc, char **argv) {
  int infd, outfd, got;
  uint32_t ne_ofs;
  const char *infn, *outfn, *ne, *data_st, *code_st, *segs;
  uint16_t sas;  /* SegAlignShift, FILE_ALIGNMENT_SHIFT */
  uint16_t stack_size;
  uint16_t entry;
  uint16_t code_size, code_minalloc;
  uint16_t data_size, data_minalloc;
  uint16_t bss_size;
  uint16_t data_idx, code_idx, stack_idx;
  uint32_t code_fofs, data_fofs;
  (void)argc; (void)argv;
  if (!argv[0] || !argv[1] || strcmp(argv[1], "-f") != 0) fail0("fatal: missing arg: -f");
  if (!argv[2] || strcmp(argv[2], "elks") != 0) fail0("fatal: missing arg: elks");
  if (!argv[3] || strcmp(argv[3], "-o") != 0) fail0("fatal: missing arg: -o");
  if (!(outfn = argv[4])) fail0("fatal: missing output file");
  if (!(infn = argv[5])) fail0("fatal: missing input file");
  if ((infd = open(infn, O_RDONLY | O_BINARY)) < 0) fail1("fatal: error opening input file: ", infn);
  if ((outfd = open(outfn, O_WRONLY | O_BINARY | O_TRUNC | O_CREAT, 0644)) < 0) fail1("fatal: error opening input file", outfn);
  memset(hdr, '\0', HDR_SIZE);
  if ((got = read(infd, hdr, HDR_SIZE)) < 0x40) fail1("fatal: input file too short: ", infn);
  if (get_u16_le(hdr) != (uint16_t)('M' | 'Z' << 8)) fail1("fatal: MZ header expected: ", infn);
  ne_ofs = get_u32_le(hdr + 0x3c);
  if (ne_ofs > HDR_SIZE - 0x40) fail1("fatal: NE ofs too large: ", infn);
  ne = hdr + ne_ofs;
  if (get_u16_le(ne) != (uint16_t)('N' | 'E' << 8)) fail1("fatal: NE header expected: ", infn);
  sas = GET_NE_SAS(ne);
  if (sas > 31) fail1("fatal: bad SegAlignShift: ", infn);
  if (get_u16_le(ne + 30) != 0) fail1("fatal: found module imports: ", infn);
  if (get_u16_le(ne + 16) != 0) fail1("fatal: bad HeapInitSize: ", infn);
  if (get_u16_le(ne + 28) != 2) fail1("fatal: bad SegCnt: ", infn);
  if (get_u16_le(ne + 48) != 0) fail1("fatal: bad MovableEntryCnt: ", infn);
  if (get_u16_le(ne + 52) != 0) fail1("fatal: bad ResourceSegCnt: ", infn);
  data_idx = GET_NE_AUTO_DATA_SEG_NO(ne);
  if (data_idx - 1U > 1U) fail1("fatal: bad AutoDataSegNo: ", infn);  /* Valid values: 1 and 2. */
  code_idx = GET_NE_INIT_CS_SEG_NO(ne);
  if (code_idx - 1U > 1U || code_idx == data_idx) fail1("fatal: bad InitCsSegNo: ", infn);  /* Valid values: 1 and 2. */
  /* stack_idx == 0 happens with OpenWatcom if there are only .c source files (no stack). */
  stack_idx = GET_NE_INIT_SS_SEG_NO(ne);
  if (stack_idx != 0 && stack_idx != data_idx) fail1("fatal: bad InitSsSegNo: ", infn);
  if (GET_NE_SEG_TAB_OFS(ne) > HDR_SIZE - ne_ofs - 0x10) fail1("fatal: SegTabOfs too large: ", infn);
  stack_size = GET_NE_STACK_SIZE(ne);
  entry = GET_NE_INIT_IP(ne);
  segs = ne + GET_NE_SEG_TAB_OFS(ne) - 8;
  data_st = segs + (data_idx << 3);
  code_st = segs + (code_idx << 3);
  if ((get_u8_le(code_st + 4) & 1) != 0) fail1("fatal: non-data code segment expected: ", infn);
  if ((get_u8_le(data_st + 4) & 1) == 0) fail1("fatal: data data segment expected: ", infn);
  if ((get_u8_le(code_st + 4 + 1) & 0xc) != 0xc) fail1("fatal: DPL3 code segment expected: ", infn);
  if ((get_u8_le(data_st + 4 + 1) & 0xc) != 0xc) fail1("fatal: DPL3 data segment expected: ", infn);
  if ((get_u8_le(code_st + 4 + 1) & 1) != 0) fail1("fatal: no-reloc code segment expected: ", infn);
  if ((get_u8_le(data_st + 4 + 1) & 1) != 0) fail1("fatal: no-reloc data segment expected: ", infn);
  code_size = get_u16_le(code_st + 2);
  code_minalloc = get_u16_le(code_st + 6);
  data_size = get_u16_le(data_st + 2);
  data_minalloc = get_u16_le(data_st + 6);
  if (code_size != code_minalloc) fail1("datal: mismatch in code size and minalloc: ", infn);
  if (data_minalloc < data_size) fail1("datal: data minalloc too small: ", infn);
  bss_size = data_minalloc - data_size;
  if (stack_idx != 0) {
    if (bss_size < stack_size) fail1("datal: bss size too small: ", infn);
    bss_size -= stack_size;
  }
  code_fofs = (uint32_t)get_u16_le(code_st) << sas;
  data_fofs = (uint32_t)get_u16_le(data_st) << sas;

  /*memset(hdr, '\0', 0x20);*/  /* ELKS a.out executable header. Not needed to clear. */
  set_u16_le(hdr, 0x0301);  /* a_magic = A_MAGIC. */
  set_u16_le(hdr + 2, 0x420);  /* a_flags = A_SEP; a_cpu = A_I8086. ELKS 0.2.0 fails if |A_FLAG.A_EXEC (as in ELKS 0.6.0) is also specified. */
  set_u16_le(hdr + 4, 0x20);  /* a_hdrlen = 0x20. */
  set_u16_le(hdr + 6, 1);  /* a_version = 1. 1 for ELKS 0.6.0 yes(1), 0 for ELKS 0.2.0 yes(1). Ignored. */
  set_u32_le(hdr + 8, code_size);  /* a_text = code_size. */
  set_u32_le(hdr + 12, data_size);  /* a_data = data_size. */
  set_u32_le(hdr + 16, bss_size);  /* a_bss = bss_size. */
  set_u32_le(hdr + 20, entry);  /* a_entry = entry. */
  set_u32_le(hdr + 24, (uint32_t)data_size + bss_size + stack_size);  /* a_total = .... */
  set_u32_le(hdr + 28, 0);  /* a_syms = 0. Unused. */

  if (write(outfd, hdr, 0x20) != 0x20) fail1("fatal: error writing header: ", outfn);
  if (lseek(infd, code_fofs, SEEK_SET) - code_fofs != 0) fail1("fatal: error seeking to code: ", infn);
  if ((got = copy_fd(infd, outfd, code_size)) == 0) {
  } else if (got == 1) {
    fail1("fatal: error reading code (file too short?): ", infn);
  } else {
    fail1("fatal: error writing code: ", outfn);
  }

  if (lseek(infd, data_fofs, SEEK_SET) - data_fofs != 0) fail1("fatal: error seeking to data: ", infn);
  if ((got = copy_fd(infd, outfd, data_size)) == 0) {
  } else if (got == 1) {
    fail1("fatal: error reading data (file too short?): ", infn);
  } else {
    fail1("fatal: error writing data: ", outfn);
  }

  /*close(outfd);*/  /* Not needed. */
  /*close(infd);*/  /* Not needed. */
  return 0;
}
