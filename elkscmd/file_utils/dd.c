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


int main(int argc, char **argv)
{
	char	*str;
	char	*cp;
	struct param	*par;
	char	*infile;
	char	*outfile;
	int	infd;
	int	outfd;
	int	incc;
	int	outcc;
	int	blocksize;
	long	count = -1;
	long	seekval;
	long	skipval;
	long	intotal = 0;
	long	outtotal = 0;
	char	*buf;
	int	retval = 1;

	infile = NULL;
	outfile = NULL;
	blocksize = 512;
	skipval = 0;
	seekval = 0;

	while (--argc > 0) {
		str = *++argv;
		cp = strchr(str, '=');
		if (cp == NULL) {
			write(STDERR_FILENO, "Missing or invalid argument(s)\n", 31);
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
					write(STDERR_FILENO, "Multiple input files illegal\n", 29);
					goto usage;
				}
	
				infile = cp;
				break;

			case PAR_OF:
				if (outfile) {
					write(STDERR_FILENO, "Multiple output files illegal\n", 30);
					goto usage;
				}

				outfile = cp;
				break;

			case PAR_BS:
				blocksize = getnum(cp);
				if (blocksize <= 0) {
					write(STDERR_FILENO, "Bad block size value\n", 21);
					goto usage;
				}
				break;

			case PAR_COUNT:
				count = getnum(cp);
				if (count < 0) {
					write(STDERR_FILENO, "Bad count value\n", 16);
					goto usage;
				}
				break;

			case PAR_SEEK:
				seekval = getnum(cp);
				if (seekval < 0) {
					write(STDERR_FILENO, "Bad seek value\n", 15);
					goto usage;
				}
				break;

			case PAR_SKIP:
				skipval = getnum(cp);
				if (skipval < 0) {
					write(STDERR_FILENO, "Bad skip value\n", 15);
					goto usage;
				}
				break;

			default:
				write(STDERR_FILENO, "Unknown dd parameter\n", 21);
				goto usage;
		}
	}

	if (infile == NULL) {
		//write(STDERR_FILENO, "No input file specified\n", 24);
		//goto usage;
		infile = "-";
	}

	if (outfile == NULL) {
		//write(STDERR_FILENO, "No output file specified\n", 25);
		//goto usage;
		outfile = "-";
	}

	buf = localbuf;
	if (blocksize > sizeof(localbuf)) {
		buf = malloc(blocksize);
		if (buf == NULL) {
			write(STDERR_FILENO, "Cannot allocate buffer\n", 23);
			goto usage;
		}
	}

	if (!strcmp(infile, "-")) 
		infd = 0;
	else {
		infd = open(infile, 0);
		if (infd < 0) {
			perror(infile);
			if (buf != localbuf) free(buf);
			goto usage;
		}
	}

	if (!strcmp(outfile, "-"))
		outfd = 1;
	else {
		outfd = creat(outfile, 0666);
		if (outfd < 0) {
			perror(outfile);
			close(infd);
			if (buf != localbuf) free(buf);
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
					write(STDERR_FILENO, "Skipped beyond end of file\n", 27);
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
	if (count > 0) count *= blocksize;
	else if (count < 0) count = 0x7fffffff;
	else goto cleanup;	/* exit immediately if count == 0 */

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
	if (incc < 0) perror(infile);
	else retval = 0;

cleanup:
	if (close(outfd) < 0) perror(outfile);
cleanup2:
	close(infd);
	if (buf != localbuf) free(buf);
	printf("%ld+%d records in\n", intotal / blocksize,
		(intotal % blocksize) != 0);
	printf("%ld+%d records out\n", outtotal / blocksize,
		(outtotal % blocksize) != 0);
	printf("%ld bytes (%ld%c KiB) copied\n", outtotal, outtotal >> 10, (outtotal & 0x3ff) ? '+' : '\0');
	return retval;

usage:
	write(STDERR_FILENO, "\nUsage: dd if=<inflie> of=<outfile> [optional_params ...]\n\n", 59);
	write(STDERR_FILENO, "Optional parameters:\n", 21);
	write(STDERR_FILENO, "bs=<blocksize>  seek=<count>  skip=<count>  count=<count>\n", 58);
	write(STDERR_FILENO, "seek/skip skips <count> blocks in input/output files, respectively\n", 67);
	write(STDERR_FILENO, "count copies only <count> blocks (default is until end of file)\n\n", 65);
	return retval;
}
