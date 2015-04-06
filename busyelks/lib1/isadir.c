#include <sys/stat.h>
#include "../sash.h"

/*
 * Return TRUE if a filename is a directory.
 * Nonexistant files return FALSE.
 */
int isadir(char *name)
{
	struct stat statbuf;
	if (stat(name, &statbuf) < 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}
