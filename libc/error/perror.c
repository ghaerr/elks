#include <errno.h>
#include <string.h>
#include <unistd.h>

void
perror(const char *str)
{
    char *ptr;

    if (str) {
        write(2, str, strlen(str));
        write(2, ": ", 2);
    }
    ptr = strerror(errno);
    write(2, ptr, strlen(ptr));
    write(2, "\n", 1);
}
