/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "grep" built-in command.
 */

#include "futils.h"

#include <ctype.h>


static	BOOL	search();

void
main(argc, argv)
	char	**argv;
{
	FILE	*fp;
	char	*word;
	char	*name;
	char	*cp;
	BOOL	tellname;
	BOOL	ignorecase;
	BOOL	tellline;
	long	line;
	char	buf[8192];

	ignorecase = FALSE;
	tellline = FALSE;

	argc--;
	argv++;

	if (**argv == '-') {
		argc--;
		cp = *argv++;

		while (*++cp) switch (*cp) {
			case 'i':
				ignorecase = TRUE;
				break;

			case 'n':
				tellline = TRUE;
				break;

			default:
				fprintf(stderr, "Unknown option\n");
				exit(1);
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

		while (fgets(buf, sizeof(buf), fp)) {

			cp = &buf[strlen(buf) - 1];
			if (*cp != '\n')
				fprintf(stderr, "%s: Line too long\n", name);

			if (search(buf, word, ignorecase)) {
				if (tellname)
					printf("%s: ", name);
				if (tellline)
					printf("%d: ", line);

				fputs(buf, stdout);
			}
		}

		if (ferror(fp))
			perror(name);

		fclose(fp);
	}
	exit(0);
}


/*
 * See if the specified word is found in the specified string.
 */
static BOOL
search(string, word, ignorecase)
	char	*string;
	char	*word;
	BOOL	ignorecase;
{
	char	*cp1;
	char	*cp2;
	int	len;
	int	lowfirst;
	int	ch1;
	int	ch2;

	len = strlen(word);

	if (!ignorecase) {
		while (TRUE) {
			string = strchr(string, word[0]);
			if (string == NULL)
				return FALSE;

			if (memcmp(string, word, len) == 0)
				return TRUE;

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

	while (TRUE) {
		while (*string && (*string != lowfirst) &&
			(!isupper(*string) || (tolower(*string) != lowfirst)))
				string++;

		if (*string == '\0')
			return FALSE;

		cp1 = string;
		cp2 = word;

		do {
			if (*cp2 == '\0')
				return TRUE;

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


/* END CODE */
