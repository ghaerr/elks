#include <stdio.h>
#include <time.h>

int main()
{
    time_t now;

    time(&now);
    fputs(ctime(&now), stdout);
    return 0;
}

