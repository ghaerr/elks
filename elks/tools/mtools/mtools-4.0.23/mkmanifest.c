/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-1998,2001,2002,2007,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * A program to create a manifest (shipping list) that is a shell script
 * to return a Unix file name to it's original state after it has been
 * clobbered by MSDOS's file name restrictions.
 *
 *	This code also used in arc, mtools, and pcomm
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"

static char *dos_name2(const char *name);

int main(int argc, char **argv)
{
	int i;
	const char *name;
	char *new_name;

	/* print the version */
	if(argc >= 2 && strcmp(argv[1], "-V") == 0) {
		printf("Mtools version %s, dated %s\n", mversion, mdate);
		return 0;
	}

	if (argc == 1) {
		fprintf(stderr, "Usage: mkmanifest [-V] <list-of-files>\n");
		return 1;
	}

	for (i=1; i<argc; i++) {
		/* zap the leading path */
		name = _basename(argv[i]);
		/* create new name */
		new_name = dos_name2(name);

		if (strcasecmp(new_name, name))
			printf("mv %s %s\n", new_name, name);
	}
	return 0;
}

static char *dos_name2(const char *name)
{
	static const char *dev[9] = {"con", "aux", "com1", "com2", "lpt1", 
				     "prn", "lpt2", "lpt3", "nul"};
	char *s;
	char *ext,*temp;
	char buf[MAX_PATH];
	int i, dot;
	static char ans[13];

	strncpy(buf, name, MAX_PATH-1);
	temp = buf;
					/* separate the name from extension */
	ext = 0;
	dot = 0;
	for (i=strlen(buf)-1; i>=0; i--) {
		if (buf[i] == '.' && !dot) {
			dot = 1;
			buf[i] = '\0';
			ext = &buf[i+1];
		}
		if (isupper((unsigned char)buf[i]))
			buf[i] = ch_tolower(buf[i]);
	}
					/* if no name */
	if (*temp == '\0')
		strcpy(ans, "x");
	else {
		/* if name is a device */
		for (i=0; i<9; i++) {
			if (!strcasecmp(temp, dev[i])) 
				*temp = 'x';
		}
		/* name too long? */
		if (strlen(temp) > 8)
			*(temp+8) = '\0';
		/* extension too long? */
		if (ext && strlen(ext) > 3)
			*(ext+3) = '\0';
		/* illegal characters? */
		while ((s = strpbrk(temp, "^+=/[]:',?*\\<>|\". ")))
			*s = 'x';

		while (ext && (s = strpbrk(ext, "^+=/[]:',?*\\<>|\". ")))
			*s = 'x';	      
		strncpy(ans, temp, 12);
		ans[12] = '\0';
	}
	if (ext && *ext) {
		strcat(ans, ".");
		strcat(ans, ext);
	}
	return(ans);
}
