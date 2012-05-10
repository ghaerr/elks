#include <sys/stat.h>
#include "../sash.h"

/*
 * Return TRUE if a filename is a directory.
 * Nonexistant files return FALSE.
 */
BOOL
isadir(name)
	char	*name;
{
	struct	stat	statbuf;

	if (stat(name, &statbuf) < 0)
		return FALSE;

	return S_ISDIR(statbuf.st_mode);
}
