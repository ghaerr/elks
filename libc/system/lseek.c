#include <unistd.h>

extern int _lseek (int fd, off_t * posn, int where);

off_t
lseek(int fd, off_t posn, int where)
{
	if ( _lseek (fd, &posn, where) < 0) return -1;
	else return posn;
}
