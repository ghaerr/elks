/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "dd" command - adapted from busybox/busyelks.
 */

#include "futils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#define	PAR_NONE	0
#define	PAR_IF		1
#define	PAR_OF		2
#define	PAR_BS		3
#define	PAR_COUNT	4
#define	PAR_SEEK	5
#define	PAR_SKIP	6


struct param {
	char	*name;
	int	value;
};

static struct param params[] = {
	{ "if",		PAR_IF },
	{ "of",		PAR_OF },
	{ "bs",		PAR_BS },
	{ "count",	PAR_COUNT },
	{ "seek",	PAR_SEEK },
	{ "skip",	PAR_SKIP },
	{ NULL,		PAR_NONE }
};

/* Fixed buffer */
static char localbuf[BUFSIZ];		/* use disk block size for efficiency*/

/*
 * Read a number with a possible multiplier.
 * Returns -1 if the number format is illegal.
 */
static long getnum(const char *cp)
{
	long value;

	if (!isdecimal(*cp))
		return -1;

	value = 0;
	while (isdecimal(*cp))
		value = value * 10 + *cp++ - '0';

	switch (*cp++) {
		case 'k':
			value *= 1024;
			break;

		case 'b':
			value *= 512;
			break;

		case 'w':
			value *= 2;
			break;

		case '\0':
			return value;

		default:
			return -1;
	}

	if (*cp) return -1;

	return value;
}

char *ultoa_r(char *buf, unsigned long i)
{
    char *b = buf + 34 - 1;

    *b = '\0';
    do {
        *--b = '0' + (i % 10);
    } while ((i /= 10) != 0);
    return b;
}
static void eprintf(const char *s, ...)
{
    va_list va;

    va_start(va, s);
    do {
        write(2, s, strlen(s));
        s = va_arg(va, const char *);
    } while (s);
    va_end(va);
}

int main(int argc, char **argv)
{
	char	*str;
	char	*cp;
	struct param	*par;
	char	*infile;
	char	*outfile;
	int	infd;
	int	outfd;
	int	incc = 0;
	int	outcc;
	int	blocksize;
	long	count = -1;
	long	seekval;
	long	skipval;
	long	intotal = 0;
	long	outtotal = 0;
	char	*buf;
	int	retval = 1;
	char	b1[34], b2[34];

	infile = NULL;
	outfile = NULL;
	blocksize = 512;
	skipval = 0;
	seekval = 0;

	while (--argc > 0) {
		str = *++argv;
		cp = strchr(str, '=');
		if (cp == NULL) {
			errmsg("Missing or invalid argument(s)\n");
			goto usage;
		}
		*cp++ = '\0';

		for (par = params; par->name; par++) {
			if (strcmp(str, par->name) == 0)
				break;
		}

		switch (par->value) {
			case PAR_IF:
				if (infile) {
					errmsg("Multiple input files illegal\n");
					goto usage;
				}
	
				infile = cp;
				break;

			case PAR_OF:
				if (outfile) {
					errmsg("Multiple output files illegal\n");
					goto usage;
				}

				outfile = cp;
				break;

			case PAR_BS:
				blocksize = getnum(cp);
				if (blocksize <= 0) {
					errmsg("Bad block size value\n");
					goto usage;
				}
				break;

			case PAR_COUNT:
				count = getnum(cp);
				if (count < 0) {
					errmsg("Bad count value\n");
					goto usage;
				}
				break;

			case PAR_SEEK:
				seekval = getnum(cp);
				if (seekval < 0) {
					errmsg("Bad seek value\n");
					goto usage;
				}
				break;

			case PAR_SKIP:
				skipval = getnum(cp);
				if (skipval < 0) {
					errmsg("Bad skip value\n");
					goto usage;
				}
				break;

			default:
				errmsg("Unknown dd parameter\n");
				goto usage;
		}
	}

	if (infile == NULL) {
		infile = "-";
	}

	if (outfile == NULL) {
		outfile = "-";
	}

	buf = localbuf;
	if (blocksize > sizeof(localbuf)) {
		buf = malloc(blocksize);
		if (buf == NULL) {
			errmsg("Cannot allocate buffer\n");
			return retval;
		}
	}

	if (!strcmp(infile, "-"))  {
		infd = 0;
		infile = "stdin";
	} else {
		infd = open(infile, 0);
		if (infd < 0) {
			perror(infile);
			goto cleanup3;
		}
	}

	if (!strcmp(outfile, "-")) {
		outfd = 1;
		outfile = "stdout";
	} else {
		outfd = creat(outfile, 0666);
		if (outfd < 0) {
			perror(outfile);
			goto cleanup2;
		}
	}

	if (skipval) {
		if (lseek(infd, skipval * blocksize, 0) < 0) {
			while (skipval-- > 0) {
				incc = read(infd, buf, blocksize);
				if (incc < 0) {
					perror(infile);
					goto cleanup;
				}

				if (incc == 0) {
					errmsg("Skipped beyond end of file\n");
					goto cleanup;
				}
			}
		}
	}

	if (seekval) {
		if (lseek(outfd, seekval * blocksize, 0) < 0) {
			perror(outfile);
			goto cleanup;
		}
	}

	/* If count is specified, only copy that many blocks */
	if (count > 0)
		count *= blocksize;
	else
		if (count < 0) count = 0x7fffffff;
	else
		goto cleanup;	/* exit immediately if count == 0 */

	while ((count > intotal) && (incc = read(infd, buf, blocksize)) > 0) {
		intotal += incc;
		cp = buf;

		while (incc > 0) {
			outcc = write(outfd, cp, incc);
			if (outcc < 0) {
				perror(outfile);
				goto cleanup;
			}

			outtotal += outcc;
			cp += outcc;
			incc -= outcc;
		}
	}

	/* Exit status can only become 0 (no error) at this point */
	if (incc < 0)
		perror(infile);
	else
		retval = 0;

cleanup:
	if (close(outfd) < 0) perror(outfile);
cleanup2:
	close(infd);
cleanup3:
	if (buf != localbuf) free(buf);

	/* %ld+%ld records in */
	eprintf(ultoa_r(b1, intotal / blocksize), "+",
		ultoa_r(b2, (intotal % blocksize) != 0), " records in\n", 0);

	/* %ld+%ld records out */
	eprintf(ultoa_r(b1, outtotal / blocksize), "+",
		ultoa_r(b2, (outtotal % blocksize) != 0), " records out\n", 0);

	/* %ld butes (%ld%c KiB) copied */
	eprintf(ultoa_r(b1, outtotal), " bytes (", ultoa_r(b2, outtotal >> 10), 0);
	if (outtotal & 0x3ff)
		errmsg("+");
	errmsg(" KiB) copied\n");
	return retval;

usage:
	errmsg("usage: dd [if=file][of=file][bs=N][count=N][seek=N][skip=N]\n");
	return 1;
}
