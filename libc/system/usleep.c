#ifdef L_usleep
#include <unistd.h>
#include <sys/time.h>

int
usleep(unsigned long useconds)
{
        struct timeval timeout;

        timeout.tv_sec	= useconds / 1000000L;
        timeout.tv_usec	= useconds % 1000000L;
        return select(1, NULL, NULL, NULL, &timeout);
}
#endif
