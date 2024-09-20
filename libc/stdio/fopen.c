#include <stdio.h>
#include <errno.h>

FILE * fopen (const char * file, const char * mode)
{
    if (file == NULL) {
        errno = EINVAL;
        return 0;
    }
    return __fopen(file, -1, (FILE *)0, mode);
}
