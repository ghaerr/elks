/*
 * Search and extract a directory structure.  The argument is the
 * relative directory entry number (no sanity checking).  It returns a
 * pointer to the directory structure at that location.  Attempts to
 * optimize by trying to determine if the buffer needs to be re-read.
 * A call to writedir() will scribble on the real buffer, so watch out!
 */

#include <stdio.h>
#include "msdos.h"
				/* dir_chain contains the list of sectors */
				/* that make up the current directory */
extern int fd, dir_chain[25];

struct directory *
search(num)
int num;
{
	int skip, entry;
	static int last;
	static struct directory dirs[16];
	void exit(), perror(), move();

					/* first call disables optimzation */
	if (num == 0)
		last = 0;
					/* which sector */
	skip = dir_chain[num / 16];
					/* don't read it if same sector */
	if (skip != last) {
		move(skip);
					/* read the sector */
		if (read(fd, (char *) &dirs[0], MSECSIZ) != MSECSIZ) {
			perror("mread: read");
			exit(1);
		}
	}
	last = skip;
					/* which entry in sector */
	entry = num % 16;
	return(&dirs[entry]);
}
