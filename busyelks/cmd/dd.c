/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "dd" built-in command.
 */

#include "../sash.h"
#include <unistd.h>

#define	PAR_NONE	0
#define	PAR_IF		1
#define	PAR_OF		2
#define	PAR_BS		3
#define	PAR_COUNT	4
#define	PAR_SEEK	5
#define	PAR_SKIP	6


typedef	struct {
	char	*name;
	int	value;
} PARAM;


static PARAM	params[] = {
	"if",		PAR_IF,
	"of",		PAR_OF,
	"bs",		PAR_BS,
	"count",	PAR_COUNT,
	"seek",		PAR_SEEK,
	"skip",		PAR_SKIP,
	NULL,		PAR_NONE
};


static	long	getnum();

void
dd_main(argc, argv)
	char	**argv;
{
	char	*str;
	char	*cp;
	PARAM	*par;
	char	*infile;
	char	*outfile;
	int	infd;
	int	outfd;
	int	incc;
	int	outcc;
	int	blocksize;
	long	count;
	long	seekval;
	long	skipval;
	long	intotal;
	long	outtotal;
	char	*buf;
	char	localbuf[8192];

	infile = NULL;
	outfile = NULL;
	seekval = 0;
	skipval = 0;
	blocksize = 512;
	count = 0x7fffffff;

	while (--argc > 0) {
		str = *++argv;
		cp = strchr(str, '=');
		if (cp == NULL) {
			write(STDERR_FILENO, "Bad dd argument\n", 16);
			return;
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
					return;
				}
	
				infile = cp;
				break;

			case PAR_OF:
				if (outfile) {
					write(STDERR_FILENO, "Multiple output files illegal\n", 30);
					return;
				}

				outfile = cp;
				break;

			case PAR_BS:
				blocksize = getnum(cp);
				if (blocksize <= 0) {
					write(STDERR_FILENO, "Bad block size value\n", 21);
					return;
				}
				break;

			case PAR_COUNT:
				count = getnum(cp);
				if (count < 0) {
					write(STDERR_FILENO, "Bad count value\n", 16);
					return;
				}
				break;

			case PAR_SEEK:
				seekval = getnum(cp);
				if (seekval < 0) {
					write(STDERR_FILENO, "Bad seek value\n", 15);
					return;
				}
				break;

			case PAR_SKIP:
				skipval = getnum(cp);
				if (skipval < 0) {
					write(STDERR_FILENO, "Bad skip value\n", 15);
					return;
				}
				break;

			default:
				write(STDERR_FILENO, "Unknown dd parameter\n", 21);
				return;
		}
	}

	if (infile == NULL) {
		write(STDERR_FILENO, "No input file specified\n", 24);
		return;
	}

	if (outfile == NULL) {
		write(STDERR_FILENO, "No output file specified\n", 25);
		return;
	}

	buf = localbuf;
	if (blocksize > sizeof(localbuf)) {
		buf = malloc(blocksize);
		if (buf == NULL) {
			write(STDERR_FILENO, "Cannot allocate buffer\n", 23);
			return;
		}
	}

	intotal = 0;
	outtotal = 0;

	infd = open(infile, 0);
	if (infd < 0) {
		perror(infile);
		if (buf != localbuf)
			free(buf);
		return;
	}

	outfd = creat(outfile, 0666);
	if (outfd < 0) {
		perror(outfile);
		close(infd);
		if (buf != localbuf)
			free(buf);
		return;
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
					write(STDERR_FILENO, "End of file while skipping\n", 27);
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

	while ((incc = read(infd, buf, blocksize)) > 0) {
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

	if (incc < 0)
		perror(infile);

cleanup:
	close(infd);

	if (close(outfd) < 0)
		perror(outfile);

	if (buf != localbuf)
		free(buf);

	printf("%d+%d records in\n", intotal / blocksize,
		(intotal % blocksize) != 0);

	printf("%d+%d records out\n", outtotal / blocksize,
		(outtotal % blocksize) != 0);

	exit(0);
}


/*
 * Read a number with a possible multiplier.
 * Returns -1 if the number format is illegal.
 */
static long
getnum(cp)
	char	*cp;
{
	long	value;

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

	if (*cp)
		return -1;

	return value;
}


/* END CODE */
