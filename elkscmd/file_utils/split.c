/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/param.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEFLINE	1000			/* default num lines per file */
#define ERR	-1			/* general error */
#define NO	0			/* no/false */
#define OK	0			/* okay exit */
#define YES	1			/* yes/true */

#define MAXBSIZE        2048
#define	isdecimal(ch)	(((ch) >= '0') && ((ch) <= '9'))

static long	bytecnt,		/* byte count to split on */
		numlines;		/* lines in each file */
static int	ifd = ERR,		/* input file descriptor */
		ofd = ERR;		/* output file descriptor */
static short	file_open;		/* if a file open */
static char	bfr[MAXBSIZE],		/* I/O buffer */
		fname[PATH_MAX];	/* file name */

void split1(void);
void split2(void);
void newfile(void);
void wrerror(void);
void usage(void);
long getnum(char *);

int main(int argc, char **argv)
{
	register int cnt;

	for (cnt = 1; cnt < argc; ++cnt) {
		if (argv[cnt][0] == '-')
			switch(argv[cnt][1]) {
			case 0:		/* stdin by request */
				if (ifd != ERR)
					usage();
				ifd = 0;
				break;
			case 'b':	/* byte count split */
				if (numlines)
					usage();
				if (!argv[cnt][2])
					bytecnt = getnum(argv[++cnt]);
				else
					bytecnt = getnum(argv[cnt] + 2);
				if (bytecnt <= 0) {
					fputs("split: byte count must be greater than zero.\n", stderr);
					usage();
				}
				break;
			case 'l':
				if (bytecnt)
					usage();
				if (!argv[cnt][2]) 
					numlines = getnum(argv[++cnt]);
				else
					numlines = getnum(argv[cnt] + 2);
				if (numlines <= 0) {
					fputs("split: line count must be greater than zero.\n", stderr);
					usage();
				}
				break;
			default:
				usage();
				break;
			}
		else if (ifd == ERR) {		/* input file */
			if ((ifd = open(argv[cnt], O_RDONLY, 0)) < 0) {
				perror(argv[cnt]);
				exit(1);
			}
		}
		else if (!*fname)		/* output file prefix */
			strcpy(fname, argv[cnt]);
		else
			usage();
	}
	if (ifd == ERR)				/* stdin by default */
		ifd = 0;
	if (bytecnt)
		split1();
	if (!numlines)
		numlines = DEFLINE;
	split2();
	exit(0);
}

/*
 * split1 --
 *	split by bytes
 */
void split1(void)
{
	register long bcnt;
	register int dist, len;
	register char *C;

	for (bcnt = 0;;) {
		switch(len = read(ifd, bfr, MAXBSIZE)) {
		case 0:
			exit(OK);
		case ERR:
			perror("read");
			exit(1);
		default:
			if (!file_open) {
				newfile();
				file_open = YES;
			}
			if (bcnt + len >= bytecnt) {
				dist = bytecnt - bcnt;
				if (write(ofd, bfr, dist) != dist)
					wrerror();
				len -= dist;
				for (C = bfr + dist; len >= bytecnt; len -= bytecnt, C += bytecnt) {
					newfile();
					if (write(ofd, C, (int)bytecnt) != bytecnt)
						wrerror();
				}
				if (len) {
					newfile();
					if (write(ofd, C, len) != len)
						wrerror();
				}
				else
					file_open = NO;
				bcnt = len;
			}
			else {
				bcnt += len;
				if (write(ofd, bfr, len) != len)
					wrerror();
			}
		}
	}
}

/*
 * split2 --
 *	split by lines
 */
void split2(void)
{
	register char *Ce, *Cs;
	register long lcnt;
	register int len, bcnt;

	for (lcnt = 0;;)
		switch(len = read(ifd, bfr, MAXBSIZE)) {
		case 0:
			exit(0);
		case ERR:
			perror("read");
			exit(1);
		default:
			if (!file_open) {
				newfile();
				file_open = YES;
			}
			for (Cs = Ce = bfr; len--; Ce++)
				if (*Ce == '\n' && ++lcnt == numlines) {
					bcnt = Ce - Cs + 1;
					if (write(ofd, Cs, bcnt) != bcnt)
						wrerror();
					lcnt = 0;
					Cs = Ce + 1;
					if (len)
						newfile();
					else
						file_open = NO;
				}
			if (Cs < Ce) {
				bcnt = Ce - Cs;
				if (write(ofd, Cs, bcnt) != bcnt)
					wrerror();
			}
		}
}

/*
 * newfile --
 *	open a new file
 */
void newfile(void)
{
	static long fnum;
	static short defname;
	static char *fpnt;

	if (ofd == ERR) {
		if (fname[0]) {
			fpnt = fname + strlen(fname);
			defname = NO;
		}
		else {
			fname[0] = 'x';
			fpnt = fname + 1;
			defname = YES;
		}
		ofd = fileno(stdout);
	}
	/*
	 * hack to increase max files; original code just wandered through
	 * magic characters.  Maximum files is 3 * 26 * 26 == 2028
	 */
#define MAXFILES	676
	if (fnum == MAXFILES) {
		if (!defname || fname[0] == 'z') {
			fputs("split: too many files.\n", stderr);
			exit(1);
		}
		++fname[0];
		fnum = 0;
	}
	fpnt[0] = fnum / 26 + 'a';
	fpnt[1] = fnum % 26 + 'a';
	++fnum;
	if (!freopen(fname, "w", stdout)) {
		fprintf(stderr, "split: unable to write to %s.\n", fname);
		exit(ERR);
	}
}

/*
 * usage --
 *	print usage message and die
 */
void usage(void)
{
	fputs("usage: split [-] [-b byte_count[k|b|w] ] [-l linecount ] [file [prefix]]\n", stderr);
	fputs("	[k|b|w] byte multiplier - 1024|512|2 respectively\n", stderr);
	exit(1);
}

/*
 * wrerror --
 *	write error
 */
void wrerror(void)
{
	perror("split: write");
	exit(1);
}

/*
 * Read a number with a possible multiplier.
 * Returns -1 if the number format is illegal.
 * (from dd.c) 
 */
long getnum(char *cp)
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
