/*
 * Convert a Unix filename to a legal MSDOS name.  Returns a pointer to
 * the 'fixed' name.  Will truncate file and extension names, will
 * substitute the letter 'X' for any illegal character in the name.
 */

#include <stdio.h>
#include <ctype.h>
#include "msdos.h"

char *
fixname(filename, verbose)
char *filename;
int verbose;
{
	static char *dev[8] = {"CON", "AUX", "COM1", "LPT1", "PRN", "LPT2",
	"LPT3", "NUL"};
	char *s, *ans, *name, *ext, *strcpy(), *strpbrk(), *strrchr();
	char buf[MAX_PATH], *malloc();
	int dot, modified;
	register int i;

	strcpy(buf, filename);
	name = buf;
					/* zap the leading path */
	if (s = strrchr(name, '/'))
		name = s+1;
	if (s = strrchr(name, '\\'))
		name = s+1;

	ext = "";
	dot = 0;
	for (i=strlen(buf)-1; i>=0; i--) {
		if (buf[i] == '.' && !dot) {
			dot = 1;
			buf[i] = '\0';
			ext = &buf[i+1];
		}
		if (islower(buf[i]))
			buf[i] = toupper(buf[i]);
	}
	if (*name == '\0') {
		name = "X";
		if (verbose)
			printf("\"%s\" Null name component, using \"%s.%s\"\n", filename, name, ext);
	}
	for (i=0; i<8; i++) {
		if (!strcmp(name, dev[i])) {
			*name = 'X';
			if (verbose)
				printf("\"%s\" Is a device name, using \"%s.%s\"\n", filename, name, ext);
		}
	}
	if (strlen(name) > 8) {
		*(name+8) = '\0';
		if (verbose)
			printf("\"%s\" Name too long, using, \"%s.%s\"\n", filename, name, ext);
	}
	if (strlen(ext) > 3) {
		*(ext+3) = '\0';
		if (verbose)
			printf("\"%s\" Extension too long, using \"%s.%s\"\n", filename, name, ext);
	}
	modified = 0;
	while (s = strpbrk(name, "^+=/[]:',?*\\<>|\". ")) {
		modified++;
		*s = 'X';
	}
	while (s = strpbrk(ext, "^+=/[]:',?*\\<>|\". ")) {
		modified++;
		*s = 'X';
	}
	if (modified && verbose)
		printf("\"%s\" Contains illegal character(s), using \"%s.%s\"\n", filename, name, ext);

	ans = malloc(12);
	sprintf(ans, "%-8.8s%-3.3s", name, ext);
	return(ans);
}
