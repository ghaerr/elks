#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	MSG	" n-seconds\n"

int
main(int argc, char **argv)
{
    int n;
    
    if (argc != 2 || (n = atoi(argv[1])) <= 0)
    {
        write(STDOUT_FILENO, argv[0], strlen(argv[0]));
        write(STDOUT_FILENO, MSG, sizeof(MSG));
        return 1;
    }

    sleep(n);
    return 0;
}
