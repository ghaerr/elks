/*
 * Test to see if a filename is a directory.  Subdir() has to be
 * run first...  Returns 1 if true.
 */

#include <stdio.h>
#include "msdos.h"

extern int dir_entries;

int
isdir(path)
char *path;
{
	register int entry;
	char *newname, *unixname(), *strncpy(), name[9], ext[4];
	struct directory *dir, *search();
	void free();
					/* no path */
	if (*path == '\0')
		return(0);

	for (entry=0; entry<dir_entries; entry++) {
		dir = search(entry);
					/* if empty */
		if (dir->name[0] == 0x0)
			break;
					/* if erased */
		if (dir->name[0] == 0xe5)
			continue;
					/* skip if not a directory */
		if (!(dir->attr & 0x10))
			continue;

		strncpy(name, (char *) dir->name, 8);
		strncpy(ext, (char *) dir->ext, 3);
		name[8] = '\0';
		ext[3] = '\0';

		newname = unixname(name, ext);
		if (!strcmp(newname, path)) {
			free(newname);
			return(1);
		}
		free(newname);
	}
					/* if "." or ".." fails, then root */
	if (!strcmp(path, ".") || !strcmp(path, ".."))
		return(1);

	return(0);
}
