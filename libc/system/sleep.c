#ifdef L_sleep
#include <time.h>
#include <sys/select.h>

unsigned int
sleep(unsigned int seconds)
{
    time_t start, stop, elapsed;
    struct timeval timeout;

    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    start = time(NULL);
    select(1, NULL, NULL, NULL, &timeout);
    stop = time(NULL);
    elapsed = stop <= start ? 0 : stop - start;
    return elapsed > seconds ? 0 : seconds - elapsed;
}

#endif
