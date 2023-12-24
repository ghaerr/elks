#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define errstr(str) write(STDERR_FILENO, str, strlen(str))

int
main(int argc, char **argv)
{
    const char *envpath;
    char path[PATH_MAX];
    int r = 1;

    if (argc < 2) {
        errstr("Usage: which cmd [...]\n");
        return 1;
    }
    if ((envpath = getenv("PATH")) == 0)
        envpath = ".";

    argv[argc] = 0;
    for (argv++; *argv; argv++) {
        const size_t file_len = strlen(*argv) + 1;
        const char *start = envpath;
        const char *end = start;

        while (1) {
            while (*end && *end != ':')
                ++end;
            size_t path_len = end - start;
            if (path_len + 1 + file_len <= sizeof(path)) {
                memcpy(path, start, path_len);
                path[path_len++] = '/';
                memcpy(path + path_len, *argv, file_len);
                path_len += file_len;

                if (access(path, F_OK) == 0) {
                    write(STDOUT_FILENO, path, path_len);
                    write(STDOUT_FILENO, "\n", 1);
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
