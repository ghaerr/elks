/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "grep" built-in command.
 */

#include "futils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUF_SIZE 4096

static char buf[BUF_SIZE];

/*
 * See if the specified word is found in the specified string.
 */
static int search(char *string, char *word, int ignorecase)
{
	char	*cp1;
	char	*cp2;
	int	len;
	int	lowfirst;
	int	ch1;
	int	ch2;

	len = strlen(word);

	if (!ignorecase) {
		while (1) {
			string = strchr(string, word[0]);
			if (string == NULL)
				return 0;

			if (memcmp(string, word, len) == 0)
				return 1;

			string++;
		}
	}

	/*
	 * Here if we need to check case independence.
	 * Do the search by lower casing both strings.
	 */
	lowfirst = *word;
	if (isupper(lowfirst))
		lowfirst = tolower(lowfirst);

	while (1) {
		while (*string && (*string != lowfirst) &&
			(!isupper(*string) || (tolower(*string) != lowfirst)))
				string++;

		if (*string == '\0')
			return 0;

		cp1 = string;
		cp2 = word;

		do {
			if (*cp2 == '\0')
				return 1;

			ch1 = *cp1++;
			if (isupper(ch1))
				ch1 = tolower(ch1);

			ch2 = *cp2++;
			if (isupper(ch2))
				ch2 = tolower(ch2);

		} while (ch1 == ch2);

		string++;
	}
}


int main(int argc, char **argv)
{
	FILE	*fp;
	char	*word;
	char	*name;
	char	*cp;
	int	tellname;
	int	ignorecase;
	int	tellline;
	long	line;

	if (argc < 2) goto usage;

	ignorecase = 0;
	tellline = 0;

	argc--;
	argv++;

	if (**argv == '-') {
		argc--;
		cp = *argv++;

		while (*++cp) switch (*cp) {
			case 'i':
				ignorecase = 1;
				break;

			case 'n':
				tellline = 1;
				break;

			default:
				fprintf(stderr, "Unknown option\n");
				return 1;
		}
	}

	word = *argv++;
	argc--;

	tellname = (argc > 1);

	while (argc-- > 0) {
		name = *argv++;

		fp = fopen(name, "r");
		if (fp == NULL) {
			perror(name);
			continue;
		}

		line = 0;

		while (fgets(buf, BUF_SIZE, fp)) {
			/* Make sure the data is text and didn't overflow */
			cp = &buf[strlen(buf) - 1];
			if (*cp != '\n') goto error_line_length;

			if (search(buf, word, ignorecase)) {
				if (tellname)
					printf("%s: ", name);
				if (tellline)
					printf("%ld: ", line);

				fputs(buf, stdout);
			}
		}

		if (ferror(fp))
			perror(name);

		fclose(fp);
	}
	return 0;

error_line_length:
	fprintf(stderr, "%s: Line too long (is this really a text file?)\n", name);
	return 1;
usage:
	fprintf(stderr, "usage: %s [-i][-n] string file1 [file2] ...\n", argv[0]);
	fprintf(stderr, "  -i: ignore upper/lower case ('z' and 'Z' match)\n");
	fprintf(stderr, "  -n: show line numbers in output\n");
	return 1;
}
