#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    const char *envpath;
    char path[PATH_MAX];
    int r = 1;

    if (argc < 2) {
        puts("Usage: which cmd [...]");
        return 1;
    }
    if ((envpath = getenv("PATH")) == 0)
        envpath = ".";

    argv[argc] = 0;
    for (argv++; *argv; argv++) {
        const size_t file_len = strlen(*argv);
        const char *start = envpath;
        const char *end = start;

        while (1) {
            while (*end && *end != ':')
                ++end;
            size_t path_len = end - start;
            if (path_len + 1 + file_len + 1 <= sizeof(path)) {
                memcpy(path, start, path_len);
                path[path_len] = '/';
                strcpy(path + path_len + 1, *argv);

                if (access(path, F_OK) == 0) {
                    puts(path);
                    r = 0;
                    break;
                }
            }

            if (!*end)
                break;
            start = end = end + 1;
        }
    }
    return r;
}
