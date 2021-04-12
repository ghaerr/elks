#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	MSG	" n-seconds\n"
#define	MSG_LEN	sizeof(MSG)

int
main(int argc, char const *argv[])
{
    int n;
    
    if (argc != 2
    || (n = atoi(argv[1])) <= 0)
    {
        write(1, argv[0], strlen(argv[0]));
        write(1, MSG, sizeof(MSG));
        return 1;
    }

    sleep(n);
    return 0;
}
