/* vi: set sw=4 ts=4: */
/*
 * od implementation for busybox
 * Based on code from util-linux v 2.11l
 *
 * Copyright (c) 1990
 * The Regents of the University of California.  All rights reserved.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *
 * Original copyright notice is retained at the end of this file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <getopt.h>
#include <sys/stat.h>

/* ELKS specific port*/
typedef int		smallint;
#define MIN(a,b)			((a) < (b)? (a): (b))
#define isprint_asciionly(a) ((unsigned)((a) - 0x20) <= 0x7e - 0x20)
#define fseeko				fseek

static void die(const char *s,...) {
	va_list p;
	va_start(p,s);
	vfprintf(stderr,s,p);
	va_end(p);
	putc('\n',stderr);  
	exit(-1);
}

static const char *skip_whitespace(const char *s)
{
	while (*s == ' ' || (unsigned char)(*s - 9) <= (13 - 9))
		s++;
	return s;
}

char *strndup(const char *s, size_t len)
{
	char *p = malloc(len + 1);
	if (p) {
		memcpy(p, (char *)s, len);
		p[len] = '\0';
	}
	return p;
}

/* dump.h*/
#define	F_IGNORE	0x01		/* %_A */
#define	F_SETREP	0x02		/* rep count set, not default */
#define	F_ADDRESS	0x001		/* print offset */
#define	F_BPAD		0x002		/* blank pad */
#define	F_C		0x004		/* %_c */
#define	F_CHAR		0x008		/* %c */
#define	F_DBL		0x010		/* %[EefGf] */
#define	F_INT		0x020		/* %[di] */
#define	F_P		0x040		/* %_p */
#define	F_STR		0x080		/* %s */
#define	F_U		0x100		/* %_u */
#define	F_UINT		0x200		/* %[ouXx] */
#define	F_TEXT		0x400		/* no conversions */

enum dump_vflag_t { ALL, DUP, FIRST, WAIT };	/* -v values */

typedef struct PR {
	struct PR *nextpr;		/* next print unit */
	unsigned flags;			/* flag values */
	int bcnt;			/* byte count */
	char *cchar;			/* conversion character */
	char *fmt;			/* printf format */
	char *nospace;			/* no whitespace version */
} PR;

typedef struct FU {
	struct FU *nextfu;		/* next format unit */
	struct PR *nextpr;		/* next print unit */
	unsigned flags;			/* flag values */
	int reps;			/* repetition count */
	int bcnt;			/* byte count */
	char *fmt;			/* format string */
} FU;

typedef struct FS {			/* format strings */
	struct FS *nextfs;		/* linked list of format strings */
	struct FU *nextfu;		/* linked list of format units */
	int bcnt;
} FS;

typedef struct dumper_t {
	off_t dump_skip;                /* bytes to skip */
	int dump_length;                /* max bytes to read */
	smallint dump_vflag;            /*enum dump_vflag_t*/
	FS *fshead;
} dumper_t;

dumper_t* alloc_dumper(void);
void bb_dump_add(dumper_t *dumper, const char *fmt);
int bb_dump_dump(dumper_t *dumper, char **argv);

/* dump.c*/


/*
 * Support code for the hexdump and od applets,
 * based on code from util-linux v 2.11l
 *
 * Copyright (c) 1989
 * The Regents of the University of California.  All rights reserved.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *
 * Original copyright notice is retained at the end of this file.
 */

static const char index_str[] = ".#-+ 0123456789";

static const char size_conv_str[] =
"\x1\x4\x4\x4\x4\x4\x4\x8\x8\x8\x8\010cdiouxXeEfgG";

static const char lcc[] = "diouxX";


typedef struct priv_dumper_t {
	dumper_t pub;

	char **argv;
	FU *endfu;
	off_t savaddress;        /* saved address/offset in stream */
	off_t eaddress;          /* end address */
	off_t address;           /* address/offset in stream */
	int blocksize;
	smallint exitval;        /* final exit value */

	/* former statics */
	smallint next__done;
	smallint get__ateof; // = 1;
	unsigned char *get__curp;
	unsigned char *get__savp;
} priv_dumper_t;

dumper_t* alloc_dumper(void)
{
	priv_dumper_t *dumper = calloc(sizeof(*dumper), 1);
	dumper->pub.dump_length = -1;
	dumper->pub.dump_vflag = FIRST;
	dumper->get__ateof = 1;
	return &dumper->pub;
}

/*
 * Utility routines.
 *
 * Copyright (C) Manuel Novoa III <mjn3@codepoet.org>
 * and Vladimir Oleynik <dzo@simtreas.ru>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

#define WANT_HEX_ESCAPES 1

/* Usual "this only works for ascii compatible encodings" disclaimer. */
#undef _tolower
#define _tolower(X) ((X)|((char) 0x20))

char bb_process_escape_sequence(const char **ptr)
{
	const char *q;
	unsigned num_digits;
	unsigned n;
	unsigned base;

	num_digits = n = 0;
	base = 8;
	q = *ptr;

	if (WANT_HEX_ESCAPES && *q == 'x') {
		++q;
		base = 16;
		++num_digits;
	}

	/* bash requires leading 0 in octal escapes:
	 * \02 works, \2 does not (prints \ and 2).
	 * We treat \2 as a valid octal escape sequence. */
	do {
		unsigned r;
#if !WANT_HEX_ESCAPES
		unsigned d = (unsigned char)(*q) - '0';
#else
		unsigned d = (unsigned char)_tolower(*q) - '0';
		if (d >= 10)
			d += ('0' - 'a' + 10);
#endif
		if (d >= base) {
			if (WANT_HEX_ESCAPES && base == 16) {
				--num_digits;
				if (num_digits == 0) {
					/* \x<bad_char>: return '\',
					 * leave ptr pointing to x */
					return '\\';
				}
			}
			break;
		}

		r = n * base + d;
		if (r > 255) {
			break;
		}

		n = r;
		++q;
	} while (++num_digits < 3);

	if (num_digits == 0) {
		/* Not octal or hex escape sequence.
		 * Is it one-letter one? */

		/* bash builtin "echo -e '\ec'" interprets \e as ESC,
		 * but coreutils "/bin/echo -e '\ec'" does not.
		 * Manpages tend to support coreutils way.
		 * Update: coreutils added support for \e on 28 Oct 2009. */
		static const char charmap[] = {
			'a',  'b', 'e', 'f',  'n',  'r',  't',  'v',  '\\', '\0',
			'\a', '\b', 27, '\f', '\n', '\r', '\t', '\v', '\\', '\\',
		};
		const char *p = charmap;
		do {
			if (*p == *q) {
				q++;
				break;
			}
		} while (*++p != '\0');
		/* p points to found escape char or NUL,
		 * advance it and find what it translates to.
		 * Note that \NUL and unrecognized sequence \z return '\'
		 * and leave ptr pointing to NUL or z. */
		n = p[sizeof(charmap) / 2];
	}

	*ptr = q;

	return (char) n;
}

char* strcpy_and_process_escape_sequences(char *dst, const char *src)
{
	while (1) {
		char c, c1;
		c = c1 = *src++;
		if (c1 == '\\')
			c1 = bb_process_escape_sequence(&src);
		*dst = c1;
		if (c == '\0')
			return dst;
		dst++;
	}
}

static int bb_dump_size(FS *fs)
{
	FU *fu;
	int bcnt, cur_size;
	char *fmt;
	const char *p;
	int prec;

	/* figure out the data block bb_dump_size needed for each format unit */
	for (cur_size = 0, fu = fs->nextfu; fu; fu = fu->nextfu) {
		if (fu->bcnt) {
			cur_size += fu->bcnt * fu->reps;
			continue;
		}
		for (bcnt = prec = 0, fmt = fu->fmt; *fmt; ++fmt) {
			if (*fmt != '%')
				continue;
			/*
			 * skip any special chars -- save precision in
			 * case it's a %s format.
			 */
			while (strchr(index_str + 1, *++fmt))
				continue;
			if (*fmt == '.' && isdigit(*++fmt)) {
				prec = atoi(fmt);
				while (isdigit(*++fmt))
					continue;
			}
			p = strchr(size_conv_str + 12, *fmt);
			if (!p) {
				if (*fmt == 's') {
					bcnt += prec;
				} else if (*fmt == '_') {
					++fmt;
					if ((*fmt == 'c') || (*fmt == 'p') || (*fmt == 'u')) {
						bcnt += 1;
					}
				}
			} else {
				bcnt += size_conv_str[p - (size_conv_str + 12)];
			}
		}
		cur_size += bcnt * fu->reps;
	}
	return cur_size;
}

static void rewrite(priv_dumper_t *dumper, FS *fs)
{
	enum { NOTOKAY, USEBCNT, USEPREC } sokay;
	FU *fu;
	PR *pr;
	char *p1, *p2, *p3;
	char savech, *fmtp;
	const char *byte_count_str;
	int nconv, prec = 0;

	for (fu = fs->nextfu; fu; fu = fu->nextfu) {
		/*
		 * break each format unit into print units; each
		 * conversion character gets its own.
		 */
		for (nconv = 0, fmtp = fu->fmt; *fmtp; ) {
			/* NOSTRICT */
			/* DBU:[dvae@cray.com] zalloc so that forward ptrs start out NULL*/
			pr = calloc(sizeof(PR), 1);
			if (!fu->nextpr)
				fu->nextpr = pr;

			/* skip preceding text and up to the next % sign */
			for (p1 = fmtp; *p1 && *p1 != '%'; ++p1)
				continue;

			/* only text in the string */
			if (!*p1) {
				pr->fmt = fmtp;
				pr->flags = F_TEXT;
				break;
			}

			/*
			 * get precision for %s -- if have a byte count, don't
			 * need it.
			 */
			if (fu->bcnt) {
				sokay = USEBCNT;
				/* skip to conversion character */
				for (++p1; strchr(index_str, *p1); ++p1)
					continue;
			} else {
				/* skip any special chars, field width */
				while (strchr(index_str + 1, *++p1))
					continue;
				if (*p1 == '.' && isdigit(*++p1)) {
					sokay = USEPREC;
					prec = atoi(p1);
					while (isdigit(*++p1))
						continue;
				} else
					sokay = NOTOKAY;
			}

			p2 = p1 + 1; /* set end pointer */

			/*
			 * figure out the byte count for each conversion;
			 * rewrite the format as necessary, set up blank-
			 * padding for end of data.
			 */
			if (*p1 == 'c') {
				pr->flags = F_CHAR;
 DO_BYTE_COUNT_1:
				byte_count_str = "\001";
 DO_BYTE_COUNT:
				if (fu->bcnt) {
					do {
						if (fu->bcnt == *byte_count_str) {
							break;
						}
					} while (*++byte_count_str);
				}
				/* Unlike the original, output the remainder of the format string. */
				if (!*byte_count_str) {
					die("bad byte count for conversion character %s", p1);
				}
				pr->bcnt = *byte_count_str;
			} else if (*p1 == 'l') {
				++p2;
				++p1;
 DO_INT_CONV:
				{
					const char *e;
					e = strchr(lcc, *p1);
					if (!e) {
						goto DO_BAD_CONV_CHAR;
					}
					pr->flags = F_INT;
					if (e > lcc + 1) {
						pr->flags = F_UINT;
					}
					byte_count_str = "\004\002\001";
					goto DO_BYTE_COUNT;
				}
				/* NOTREACHED */
			} else if (strchr(lcc, *p1)) {
				goto DO_INT_CONV;
			} else if (strchr("eEfgG", *p1)) {
				pr->flags = F_DBL;
				byte_count_str = "\010\004";
				goto DO_BYTE_COUNT;
			} else if (*p1 == 's') {
				pr->flags = F_STR;
				if (sokay == USEBCNT) {
					pr->bcnt = fu->bcnt;
				} else if (sokay == USEPREC) {
					pr->bcnt = prec;
				} else {   /* NOTOKAY */
					die("%%s requires a precision or a byte count");
				}
			} else if (*p1 == '_') {
				++p2;
				switch (p1[1]) {
				case 'A':
					dumper->endfu = fu;
					fu->flags |= F_IGNORE;
					/* FALLTHROUGH */
				case 'a':
					pr->flags = F_ADDRESS;
					++p2;
					if ((p1[2] != 'd') && (p1[2] != 'o') && (p1[2] != 'x')) {
						goto DO_BAD_CONV_CHAR;
					}
					*p1 = p1[2];
					break;
				case 'c':
					pr->flags = F_C;
					/* *p1 = 'c';   set in conv_c */
					goto DO_BYTE_COUNT_1;
				case 'p':
					pr->flags = F_P;
					*p1 = 'c';
					goto DO_BYTE_COUNT_1;
				case 'u':
					pr->flags = F_U;
					/* *p1 = 'c';   set in conv_u */
					goto DO_BYTE_COUNT_1;
				default:
					goto DO_BAD_CONV_CHAR;
				}
			} else {
 DO_BAD_CONV_CHAR:
				die("bad conversion character %%%s", p1);
			}

			/*
			 * copy to PR format string, set conversion character
			 * pointer, update original.
			 */
			savech = *p2;
			p1[1] = '\0';
			pr->fmt = strdup(fmtp);
			*p2 = savech;
			//Too early! realloc can move pr->fmt!
			//pr->cchar = pr->fmt + (p1 - fmtp);

			/* DBU:[dave@cray.com] w/o this, trailing fmt text, space is lost.
			 * Skip subsequent text and up to the next % sign and tack the
			 * additional text onto fmt: eg. if fmt is "%x is a HEX number",
			 * we lose the " is a HEX number" part of fmt.
			 */
			for (p3 = p2; *p3 && *p3 != '%'; p3++)
				continue;
			if (p3 > p2) {
				savech = *p3;
				*p3 = '\0';
				pr->fmt = realloc(pr->fmt, strlen(pr->fmt) + (p3-p2) + 1);
				strcat(pr->fmt, p2);
				*p3 = savech;
				p2 = p3;
			}

			pr->cchar = pr->fmt + (p1 - fmtp);
			fmtp = p2;

			/* only one conversion character if byte count */
			if (!(pr->flags & F_ADDRESS) && fu->bcnt && nconv++) {
				die("byte count with multiple conversion characters");
			}
		}
		/*
		 * if format unit byte count not specified, figure it out
		 * so can adjust rep count later.
		 */
		if (!fu->bcnt)
			for (pr = fu->nextpr; pr; pr = pr->nextpr)
				fu->bcnt += pr->bcnt;
	}
	/*
	 * if the format string interprets any data at all, and it's
	 * not the same as the blocksize, and its last format unit
	 * interprets any data at all, and has no iteration count,
	 * repeat it as necessary.
	 *
	 * if rep count is greater than 1, no trailing whitespace
	 * gets output from the last iteration of the format unit.
	 */
	for (fu = fs->nextfu; fu; fu = fu->nextfu) {
		if (!fu->nextfu
		 && fs->bcnt < dumper->blocksize
		 && !(fu->flags & F_SETREP)
		 && fu->bcnt
		) {
			fu->reps += (dumper->blocksize - fs->bcnt) / fu->bcnt;
		}
		if (fu->reps > 1 && fu->nextpr) {
			for (pr = fu->nextpr;; pr = pr->nextpr)
				if (!pr->nextpr)
					break;
			for (p1 = pr->fmt, p2 = NULL; *p1; ++p1)
				p2 = isspace(*p1) ? p1 : NULL;
			if (p2)
				pr->nospace = p2;
		}
		if (!fu->nextfu)
			break;
	}
}

static void do_skip(priv_dumper_t *dumper, const char *fname, int statok)
{
	struct stat sbuf;

	if (statok) {
		if (!strcmp(fname, "stdin"))
			fstat(STDIN_FILENO, &sbuf);
		else
			stat(fname, &sbuf);
		if (!(S_ISCHR(sbuf.st_mode) || S_ISBLK(sbuf.st_mode) || S_ISFIFO(sbuf.st_mode))
		 && dumper->pub.dump_skip >= sbuf.st_size
		) {
			/* If bb_dump_size valid and pub.dump_skip >= size */
			dumper->pub.dump_skip -= sbuf.st_size;
			dumper->address += sbuf.st_size;
			return;
		}
	}
	if (fseeko(stdin, dumper->pub.dump_skip, SEEK_SET)) {
		die(fname);
	}
	dumper->address += dumper->pub.dump_skip;
	dumper->savaddress = dumper->address;
	dumper->pub.dump_skip = 0;
}

static int next(priv_dumper_t *dumper)
{
	int statok;

	for (;;) {
		if (*dumper->argv) {
			dumper->next__done = statok = 1;
			if (!(freopen(*dumper->argv, "r", stdin))) {
				perror(*dumper->argv);
				dumper->exitval = 1;
				++dumper->argv;
				continue;
			}
		} else {
			if (dumper->next__done)
				return 0; /* no next file */
			dumper->next__done = 1;
			statok = 0;
		}
		if (dumper->pub.dump_skip)
			do_skip(dumper, statok ? *dumper->argv : "stdin", statok);
		if (*dumper->argv)
			++dumper->argv;
		if (!dumper->pub.dump_skip)
			return 1;
	}
	/* NOTREACHED */
}

static unsigned char *get(priv_dumper_t *dumper)
{
	int n;
	int need, nread;
	int blocksize = dumper->blocksize;

	if (!dumper->get__curp) {
		dumper->address = (off_t)0; /*DBU:[dave@cray.com] initialize,initialize..*/
		dumper->get__curp = malloc(blocksize);
		dumper->get__savp = calloc(blocksize, 1); /* need to be initialized */
	} else {
		unsigned char *tmp = dumper->get__curp;
		dumper->get__curp = dumper->get__savp;
		dumper->get__savp = tmp;
		dumper->savaddress += blocksize;
		dumper->address = dumper->savaddress;
	}
	need = blocksize;
	nread = 0;
	while (1) {
		/*
		 * if read the right number of bytes, or at EOF for one file,
		 * and no other files are available, zero-pad the rest of the
		 * block and set the end flag.
		 */
		if (!dumper->pub.dump_length || (dumper->get__ateof && !next(dumper))) {
			if (need == blocksize) {
				return NULL;
			}
			if (dumper->pub.dump_vflag != ALL && !memcmp(dumper->get__curp, dumper->get__savp, nread)) {
				if (dumper->pub.dump_vflag != DUP) {
					puts("*");
				}
				return NULL;
			}
			memset(dumper->get__curp + nread, 0, need);
			dumper->eaddress = dumper->address + nread;
			return dumper->get__curp;
		}
		n = fread(dumper->get__curp + nread, sizeof(unsigned char),
				dumper->pub.dump_length == -1 ? need : MIN(dumper->pub.dump_length, need), stdin);
		if (!n) {
			if (ferror(stdin)) {
				perror(dumper->argv[-1]);
			}
			dumper->get__ateof = 1;
			continue;
		}
		dumper->get__ateof = 0;
		if (dumper->pub.dump_length != -1) {
			dumper->pub.dump_length -= n;
		}
		need -= n;
		if (!need) {
			if (dumper->pub.dump_vflag == ALL || dumper->pub.dump_vflag == FIRST
			 || memcmp(dumper->get__curp, dumper->get__savp, blocksize)
			) {
				if (dumper->pub.dump_vflag == DUP || dumper->pub.dump_vflag == FIRST) {
					dumper->pub.dump_vflag = WAIT;
				}
				return dumper->get__curp;
			}
			if (dumper->pub.dump_vflag == WAIT) {
				puts("*");
			}
			dumper->pub.dump_vflag = DUP;
			dumper->savaddress += blocksize;
			dumper->address = dumper->savaddress;
			need = blocksize;
			nread = 0;
		} else {
			nread += n;
		}
	}
}

static void bpad(PR *pr)
{
	char *p1, *p2;

	/*
	 * remove all conversion flags; '-' is the only one valid
	 * with %s, and it's not useful here.
	 */
	pr->flags = F_BPAD;
	*pr->cchar = 's';
	for (p1 = pr->fmt; *p1 != '%'; ++p1)
		continue;
	for (p2 = ++p1; *p1 && strchr(" -0+#", *p1); ++p1)
		if (pr->nospace)
			pr->nospace--;
	while ((*p2++ = *p1++) != 0)
		continue;
}

static const char conv_str[] =
	"\0"  "\\""0""\0"
	"\007""\\""a""\0"  /* \a */
	"\b"  "\\""b""\0"
	"\f"  "\\""f""\0"
	"\n"  "\\""n""\0"
	"\r"  "\\""r""\0"
	"\t"  "\\""t""\0"
	"\v"  "\\""v""\0"
	;


static void conv_c(PR *pr, unsigned char *p)
{
	const char *str = conv_str;
	char buf[10];

	do {
		if (*p == *str) {
			++str;
			goto strpr; /* map e.g. '\n' to "\\n" */
		}
		str += 4;
	} while (*str);

	if (isprint_asciionly(*p)) {
		*pr->cchar = 'c';
		printf(pr->fmt, *p);
	} else {
		sprintf(buf, "%03o", (int) *p);
		str = buf;
 strpr:
		*pr->cchar = 's';
		printf(pr->fmt, str);
	}
}

static void conv_u(PR *pr, unsigned char *p)
{
	static const char list[] =
		"nul\0soh\0stx\0etx\0eot\0enq\0ack\0bel\0"
		"bs\0_ht\0_lf\0_vt\0_ff\0_cr\0_so\0_si\0_"
		"dle\0dcl\0dc2\0dc3\0dc4\0nak\0syn\0etb\0"
		"can\0em\0_sub\0esc\0fs\0_gs\0_rs\0_us";

	/* od used nl, not lf */
	if (*p <= 0x1f) {
		*pr->cchar = 's';
		printf(pr->fmt, list + (4 * (int)*p));
	} else if (*p == 0x7f) {
		*pr->cchar = 's';
		printf(pr->fmt, "del");
	} else if (*p < 0x7f) { /* isprint() */
		*pr->cchar = 'c';
		printf(pr->fmt, *p);
	} else {
		*pr->cchar = 'x';
		printf(pr->fmt, (int) *p);
	}
}

static void display(priv_dumper_t* dumper)
{
	FS *fs;
	FU *fu;
	PR *pr;
	int cnt;
	unsigned char *bp, *savebp;
	off_t saveaddress;
	unsigned char savech = '\0';

	while ((bp = get(dumper)) != NULL) {
		fs = dumper->pub.fshead;
		savebp = bp;
		saveaddress = dumper->address;
		for (; fs; fs = fs->nextfs, bp = savebp, dumper->address = saveaddress) {
			for (fu = fs->nextfu; fu; fu = fu->nextfu) {
				if (fu->flags & F_IGNORE) {
					break;
				}
				for (cnt = fu->reps; cnt; --cnt) {
					for (pr = fu->nextpr; pr; dumper->address += pr->bcnt,
								bp += pr->bcnt, pr = pr->nextpr) {
						if (dumper->eaddress && dumper->address >= dumper->eaddress
						 && !(pr->flags & (F_TEXT | F_BPAD))
						) {
							bpad(pr);
						}
						if (cnt == 1 && pr->nospace) {
							savech = *pr->nospace;
							*pr->nospace = '\0';
						}
/*                      PRINT; */
						switch (pr->flags) {
						case F_ADDRESS:
							printf(pr->fmt, (unsigned) dumper->address);
							break;
						case F_BPAD:
							printf(pr->fmt, "");
							break;
						case F_C:
							conv_c(pr, bp);
							break;
						case F_CHAR:
							printf(pr->fmt, *bp);
							break;
						case F_DBL: {
							double dval;
							float fval;

							switch (pr->bcnt) {
							case 4:
								memcpy(&fval, bp, sizeof(fval));
								printf(pr->fmt, fval);
								break;
							case 8:
								memcpy(&dval, bp, sizeof(dval));
								printf(pr->fmt, dval);
								break;
							}
							break;
						}
						case F_INT: {
							int ival;
							short sval;

							switch (pr->bcnt) {
							case 1:
								printf(pr->fmt, (int) *bp);
								break;
							case 2:
								memcpy(&sval, bp, sizeof(sval));
								printf(pr->fmt, (int) sval);
								break;
							case 4:
								memcpy(&ival, bp, sizeof(ival));
								printf(pr->fmt, ival);
								break;
							}
							break;
						}
						case F_P:
							printf(pr->fmt, isprint_asciionly(*bp) ? *bp : '.');
							break;
						case F_STR:
							printf(pr->fmt, (char *) bp);
							break;
						case F_TEXT:
							printf(pr->fmt);
							break;
						case F_U:
							conv_u(pr, bp);
							break;
						case F_UINT: {
							unsigned ival;
							unsigned short sval;

							switch (pr->bcnt) {
							case 1:
								printf(pr->fmt, (unsigned) *bp);
								break;
							case 2:
								memcpy(&sval, bp, sizeof(sval));
								printf(pr->fmt, (unsigned) sval);
								break;
							case 4:
								memcpy(&ival, bp, sizeof(ival));
								printf(pr->fmt, ival);
								break;
							}
							break;
						}
						}
						if (cnt == 1 && pr->nospace) {
							*pr->nospace = savech;
						}
					}
				}
			}
		}
	}
	if (dumper->endfu) {
		/*
		 * if eaddress not set, error or file size was multiple
		 * of blocksize, and no partial block ever found.
		 */
		if (!dumper->eaddress) {
			if (!dumper->address) {
				return;
			}
			dumper->eaddress = dumper->address;
		}
		for (pr = dumper->endfu->nextpr; pr; pr = pr->nextpr) {
			switch (pr->flags) {
			case F_ADDRESS:
				printf(pr->fmt, (unsigned) dumper->eaddress);
				break;
			case F_TEXT:
				printf(pr->fmt);
				break;
			}
		}
	}
}

#define dumper ((priv_dumper_t*)pub_dumper)
int bb_dump_dump(dumper_t *pub_dumper, char **argv)
{
	FS *tfs;
	int blocksize;

	/* figure out the data block bb_dump_size */
	blocksize = 0;
	tfs = dumper->pub.fshead;
	while (tfs) {
		tfs->bcnt = bb_dump_size(tfs);
		if (blocksize < tfs->bcnt) {
			blocksize = tfs->bcnt;
		}
		tfs = tfs->nextfs;
	}
	dumper->blocksize = blocksize;

	/* rewrite the rules, do syntax checking */
	for (tfs = dumper->pub.fshead; tfs; tfs = tfs->nextfs) {
		rewrite(dumper, tfs);
	}

	dumper->argv = argv;
	display(dumper);

	return dumper->exitval;
}

void bb_dump_add(dumper_t* pub_dumper, const char *fmt)
{
	const char *p;
	FS *tfs;
	FU *tfu, **nextfupp;
	const char *savep;

	/* start new linked list of format units */
	tfs = calloc(sizeof(FS), 1); /*DBU:[dave@cray.com] start out NULL */
	if (!dumper->pub.fshead) {
		dumper->pub.fshead = tfs;
	} else {
		FS *fslast = dumper->pub.fshead;
		while (fslast->nextfs)
			fslast = fslast->nextfs;
		fslast->nextfs = tfs;
	}
	nextfupp = &tfs->nextfu;

	/* take the format string and break it up into format units */
	p = fmt;
	for (;;) {
		p = skip_whitespace(p);
		if (*p == '\0') {
			break;
		}

		/* allocate a new format unit and link it in */
		/* NOSTRICT */
		/* DBU:[dave@cray.com] zalloc so that forward pointers start out NULL */
		tfu = calloc(sizeof(FU), 1);
		*nextfupp = tfu;
		nextfupp = &tfu->nextfu;
		tfu->reps = 1;

		/* if leading digit, repetition count */
		if (isdigit(*p)) {
			for (savep = p; isdigit(*p); ++p)
				continue;
			if (!isspace(*p) && *p != '/') {
				die("bad format {%s}", fmt);
			}
			/* may overwrite either white space or slash */
			tfu->reps = atoi(savep);
			tfu->flags = F_SETREP;
			/* skip trailing white space */
			p = skip_whitespace(++p);
		}

		/* skip slash and trailing white space */
		if (*p == '/') {
			p = skip_whitespace(p + 1);
		}

		/* byte count */
		if (isdigit(*p)) {
// TODO: use bb_strtou
			savep = p;
			while (isdigit(*++p))
				continue;
			if (!isspace(*p)) {
				die("bad format {%s}", fmt);
			}
			tfu->bcnt = atoi(savep);
			/* skip trailing white space */
			p = skip_whitespace(p + 1);
		}

		/* format */
		if (*p != '"') {
			die("bad format {%s}", fmt);
		}
		for (savep = ++p; *p != '"';) {
			if (*p++ == '\0') {
				die("bad format {%s}", fmt);
			}
		}
		tfu->fmt = strndup(savep, p - savep);

		/* alphabetic escape sequences have to be done in place */
		strcpy_and_process_escape_sequences(tfu->fmt, tfu->fmt);
		/* unknown mappings are not changed: "\z" -> '\\' 'z' */
		/* trailing backslash, if any, is preserved */
#if 0
		char *p1;
		char *p2;
		p1 = tfu->fmt;
		for (p2 = p1;; ++p1, ++p2) {
			*p2 = *p1;
			if (*p1 == '\0')
				break;

			if (*p1 == '\\') {
				const char *cs;

				p1++;
				*p2 = *p1;
				if (*p1 == '\0') {
					/* "...\" trailing backslash. Eaten. */
					break;
				}
				cs = conv_str + 4; /* skip NUL element */
				do {
					/* map e.g. "\n" -> '\n' */
					if (*p1 == cs[2]) {
						*p2 = cs[0];
						break;
					}
					cs += 4;
				} while (*cs);
				/* unknown mappings remove bkslash: "\z" -> 'z' */
			}
		}
#endif

		p++;
	}
}
#undef dumper

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* od.c*/
static void
odoffset(dumper_t *dumper, int argc, char ***argvp)
{
	char *num, *p;
	int base;
	char *end;

	/*
	 * The offset syntax of od(1) was genuinely bizarre.  First, if
	 * it started with a plus it had to be an offset.  Otherwise, if
	 * there were at least two arguments, a number or lower-case 'x'
	 * followed by a number makes it an offset.  By default it was
	 * octal; if it started with 'x' or '0x' it was hex.  If it ended
	 * in a '.', it was decimal.  If a 'b' or 'B' was appended, it
	 * multiplied the number by 512 or 1024 byte units.  There was
	 * no way to assign a block count to a hex offset.
	 *
	 * We assumes it's a file if the offset is bad.
	 */
	p = **argvp;

	if (!p) {
		/* hey someone is probably piping to us ... */
		return;
	}

	if ((*p != '+')
		&& (argc < 2
			|| (!isdigit(p[0])
				&& ((p[0] != 'x') || !isxdigit(p[1])))))
		return;

	base = 0;
	/*
	 * skip over leading '+', 'x[0-9a-fA-f]' or '0x', and
	 * set base.
	 */
	if (p[0] == '+')
		++p;
	if (p[0] == 'x' && isxdigit(p[1])) {
		++p;
		base = 16;
	} else if (p[0] == '0' && p[1] == 'x') {
		p += 2;
		base = 16;
	}

	/* skip over the number */
	if (base == 16)
		for (num = p; isxdigit(*p); ++p)
			continue;
	else
		for (num = p; isdigit(*p); ++p)
			continue;

	/* check for no number */
	if (num == p)
		return;

	/* if terminates with a '.', base is decimal */
	if (*p == '.') {
		if (base)
			return;
		base = 10;
	}

	dumper->dump_skip = strtol(num, &end, base ? base : 8);

	/* if end isn't the same as p, we got a non-octal digit */
	if (end != p)
		dumper->dump_skip = 0;
	else {
		if (*p) {
			if (*p == 'b') {
				dumper->dump_skip *= 512;
				++p;
			} else if (*p == 'B') {
				dumper->dump_skip *= 1024;
				++p;
			}
		}
		if (*p)
			dumper->dump_skip = 0;
		else {
			++*argvp;
			/*
			 * If the offset uses a non-octal base, the base of
			 * the offset is changed as well.  This isn't pretty,
			 * but it's easy.
			 */
#define TYPE_OFFSET 7
			{
				char x_or_d;
				if (base == 16) {
					x_or_d = 'x';
					goto DO_X_OR_D;
				}
				if (base == 10) {
					x_or_d = 'd';
 DO_X_OR_D:
					dumper->fshead->nextfu->fmt[TYPE_OFFSET]
						= dumper->fshead->nextfs->nextfu->fmt[TYPE_OFFSET]
						= x_or_d;
				}
			}
		}
	}
}

static const char *const add_strings[] = {
	"16/1 \"%3_u \" \"\\n\"",              /* a */
	"8/2 \" %06o \" \"\\n\"",              /* B, o */
	"16/1 \"%03o \" \"\\n\"",              /* b */
	"16/1 \"%3_c \" \"\\n\"",              /* c */
	"8/2 \"  %05u \" \"\\n\"",             /* d */
	"4/4 \"     %010u \" \"\\n\"",         /* D */
	"2/8 \"          %21.14e \" \"\\n\"",  /* e (undocumented in od), F */
	"4/4 \" %14.7e \" \"\\n\"",            /* f */
	"4/4 \"       %08x \" \"\\n\"",        /* H, X */
	"8/2 \"   %04x \" \"\\n\"",            /* h, x */
	"4/4 \"    %11d \" \"\\n\"",           /* I, L, l */
	"8/2 \" %6d \" \"\\n\"",               /* i */
	"4/4 \"    %011o \" \"\\n\"",          /* O */
};

static const char od_opts[] = "aBbcDdeFfHhIiLlOoXxv";

static const char od_o2si[] = {
	0, 1, 2, 3, 5,
	4, 6, 6, 7, 8,
	9, 0xa, 0xb, 0xa, 0xa,
	0xb, 1, 8, 9,
};

int main(int argc, char **argv)
{
	int ch;
	int first = 1;
	char *p;
	dumper_t *dumper = alloc_dumper();

	while ((ch = getopt(argc, argv, od_opts)) > 0) {
		if (ch == 'v') {
			dumper->dump_vflag = ALL;
		} else if (((p = strchr(od_opts, ch)) != NULL) && (*p != '\0')) {
			if (first) {
				first = 0;
				bb_dump_add(dumper, "\"%07.7_Ao\n\"");
				bb_dump_add(dumper, "\"%07.7_ao  \"");
			} else {
				bb_dump_add(dumper, "\"         \"");
			}
			bb_dump_add(dumper, add_strings[(int)od_o2si[(p - od_opts)]]);
		} else {  /* P, p, s, w, or other unhandled */
			die("usage: [-aBbcDdeFfHhIiLlOovXx] [FILE]\n");
		}
	}
	if (!dumper->fshead) {
		bb_dump_add(dumper, "\"%07.7_Ao\n\"");
		bb_dump_add(dumper, "\"%07.7_ao  \" 8/2 \"%06o \" \"\\n\"");
	}

	argc -= optind;
	argv += optind;

	odoffset(dumper, argc, &argv);

	return bb_dump_dump(dumper, argv);
}

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
