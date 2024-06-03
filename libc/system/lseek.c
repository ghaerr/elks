#include <unistd.h>

off_t
lseek(int fd, off_t posn, int where)
{
    if (_lseek (fd, &posn, where) < 0)
        return -1;
    return posn;
}
