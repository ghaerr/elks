/*
 * unixname(), getname(), getpath()
 */

#include <stdio.h>
#include <ctype.h>
#include "msdos.h"

/*
 * Get rid of spaces in a MSDOS 'raw' name (one that has come from the
 * directory structure) so that it can be used for regular expression
 * matching with a unix file name.  Also used to 'unfix' a name that has
 * been altered by fixname().  Returns a pointer to the unix style name.
 */

char *
unixname(name, ext)
char *name, *ext;
{
	char *s, tname[9], text[4], *strcpy(), *strcat(), *strchr();
	char *ans, *malloc();

	strcpy(tname, name);
	if (s = strchr(tname, ' '))
		*s = '\0';

	strcpy(text, ext);
	if (s = strchr(text, ' '))
		*s = '\0';

	ans = malloc(13);

	if (*text) {
		strcpy(ans, tname);
		strcat(ans, ".");
		strcat(ans, text);
	}
	else
		strcpy(ans, tname);
	return(ans);
}

/*
 * Get name component of filename.  Translates name to upper case.  Returns
 * pointer to new name.
 */

char *
getname(filename)
char *filename;
{
	char *s, *ans, *malloc(), *temp, *strcpy(), *strrchr();
	char buf[MAX_PATH];

	strcpy(buf, filename);
	temp = buf;
					/* find the last separator */
	if (s = strrchr(temp, '/'))
		temp = s+1;
	if (s = strrchr(temp, '\\'))
		temp = s+1;
					/* xlate to upper case */
	for (s = temp; *s; ++s) {
		if (islower(*s))
			*s = toupper(*s);
	}

	ans = malloc((unsigned int) strlen(temp)+1);
	strcpy(ans, temp);
	return(ans);
}

/*
 * Get the path component of the filename.  Translates to upper case.
 * Returns pointer to path.  Doesn't alter leading separator, always
 * strips trailing separator (unless it is the path itself).
 */

char *
getpath(filename)
char *filename;
{
	char *s, *temp, *ans, *malloc(), *strcpy(), *strrchr();
	char buf[MAX_PATH];
	int has_sep;

	strcpy(buf, filename);
	temp = buf;
					/* find last separator */
	has_sep = 0;
	if (s = strrchr(temp, '/')) {
		has_sep++;
		temp = s;
	}
	if (s = strrchr(temp, '\\')) {
		has_sep++;
		temp = s;
	}

	*temp = '\0';
					/* translate to upper case */
	for (s = buf; *s; ++s) {
		if (islower(*s))
			*s = toupper(*s);
	}
					/* if separator alone, put it back */
	if (!strlen(buf) && has_sep)
		strcpy(buf, "/");

	ans = malloc((unsigned int) strlen(buf)+1);
	strcpy(ans, buf);
	return(ans);
}
