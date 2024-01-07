/*
 * tr.c
 *
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

/*
 * This is a small version of tr for use in the ELKS project.
 * It is not fully functional, and may not be the most efficient
 * implementation for larger systems. It minimises memory usage and
 * code size.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

int truncate1 = 0;	/* Truncate set1 to set2 length (SYSV) */
int complement1 = 0;	/* Complement set1 */
int squeeze = 0;
int delete = 0;

char * progname;

/*
 *
 * usage()
 *
 * Display usage information.
 *
 */

void usage(char ** argv)
{
	fprintf(stderr, "%s [-cstd] string1 [string2]\n", argv[0]);
	exit(1);
}

/*
 *
 * do_args()
 *
 * Interpret command switches from the command line.
 *
 * int argc;		As passed to main().
 * char ** argv;	As passed to main().
 *
 * RETURN		Num of arguments interpreted.
 *
 */

int do_args(int argc, char ** argv)
{
	int i = 1;
	size_t j;

	while (i < argc && argv[i][0] == '-') {
		for (j = 1; j < strlen(argv[i]); j++) {
			switch (argv[i][j]) {
				case 'c':
					complement1 = 1; /* TODO implement */
					break;
				case 's':
					squeeze = 1;
					break;
				case 't':
					truncate1 = 1;
					break;
				case 'd':
					delete = 1;
					break;
				default:
					usage(argv);
			}
		}
		i++;
	}
	switch (argc - i) {
		case 1:
			if (truncate1 || !(delete ^ squeeze)) {
				usage(argv);
			}
			break;
		case 2:
			if (delete && truncate1) {
				usage(argv);
			}
			break;
		default:
			usage(argv);
	}
	return i;
}

void out_of_mem()
{
	perror("malloc");
	exit(1);
}

void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL)
		out_of_mem();
	return ptr;
}

/*
 *
 * build_string()
 *
 * Build a full string based on argument parsed.
 *
 * char * set_descr;	Argument describing character set.
 * int num;		Number of set being built (1 or 2).
 *
 */

#define BSIZE 64

char * build_string(char * set_descr, int num)
{
	char * buf = calloc(1, BSIZE);
	int size = BSIZE;
	int tail = 0;
	size_t i;
	char ch, n1, n2;

	if (buf == NULL) {
		out_of_mem();
	}

	for (i = 0; i < strlen(set_descr); i++) {
		ch = set_descr[i];
again:
		switch (ch) {
			case '\\':
				ch = set_descr[++i];
				switch (ch) {
					case 'b':
						ch = '\b';
						goto again;
					case 't':
						ch = '\t';
						goto again;
					case 'n':
						ch = '\n';
						goto again;
					case 'v':
						ch = '\v';
						goto again;
					case 'f':
						ch = '\t';
						goto again;
					case 'r':
						ch = '\r';
						goto again;
					case '\\':
						ch = '\\';
						goto again;
					case '0':
						if (((n1 = set_descr[++i]) > 47)
						 && (n1 < 58)
						 && ((n2 = set_descr[++i]) > 47)
						 && (n2 < 58)) {
							n1 -= 48;
							n2 -= 48;
							ch = n1 * 8 + n2;
							goto again;
						}
						/* fallthrough */
					default:
						fprintf(stderr, "%s: Illegal %c in set%d\n", progname, ch, num);
						exit(1);

				}
				break;
			case '-':
				n1 = ' ';
				if ((tail) &&
				    ((n1 = set_descr[++i]) >= (n2 = buf[tail-1] + 1))) {
					int need = n1 - n2 + 1;
					if (size - tail < need + 4) {
						buf = xrealloc(buf, size + need);
						memset(buf + size, 0, need);
						size += need;
					}
					for (;n2 <= n1;n2++){
						buf[tail++] = n2;
					}
				} else {
					fprintf(stderr, "%s: Illegal %c-%c in set%d\n", progname, buf[tail-1], n1, num);
				}
				break;
			default:
				buf[tail++] = ch;
				break;

		}
		if ((size - tail) < 4) {
			buf = xrealloc(buf, size + BSIZE);
			memset(buf + size, 0, BSIZE);
			size += BSIZE;
		}
	}
	return buf;
}

int main(int argc, char ** argv)
{
	int num_args, i, lchar = 0;
	char * set1, * set2;
	char * ip;
	int len1, len2;

	progname = argv[0];
	num_args = do_args(argc, argv);

	set1 = build_string(argv[num_args++], 1);
	len1 = strlen(set1);
	if (num_args < argc) {
		set2 = build_string(argv[num_args], 2);
		len2 = strlen(set2);
	} else {
		set2 = NULL;
		len2 = 0;
	}
	/* printf("{%d,%d}",len1,len2); */
	if (set2 && len1 > len2) {
		if (truncate1) {
			set1[len2] = '\0';
		} else {
			char pad = len2 ? set2[len2 - 1] : '\0';
			set2 = xrealloc(set2, len1 + 1);
			for (i = len2; i < len1; i++) {
				set2[i] = pad;
			}
			set2[len1] = 0;
		}
	}
	/* printf("String 1 = %s\n", set1);
	printf("String 2 = %s\n", set2); */
	while ((i = getchar()) != EOF) {
		if (delete) {
			if (strchr(set1, i) != NULL) {
				i = 0;
			}
		} else if (set2) {
			if ((ip = strchr(set1, i)) != NULL) {
				i = ip - set1;
				ip = set2 + i;
				i = *ip;
			}
		}
		if (squeeze) {
			if ((set2 && (strchr(set2, i) != NULL)) ||
			   (!set2 && (strchr(set1, i) != NULL))) {
				if (i == lchar) i = 0;
			}
		}
		if (i) {
			lchar = i;
			putchar(i);
		}
	}
	exit(0);
}
