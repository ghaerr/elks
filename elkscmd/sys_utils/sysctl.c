/*
 * sysctl - system control utility for ELKS
 *
 * 8 May 2024 Greg Haerr
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/sysctl.h>

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

void msg(int fd, const char *s, ...)
{
    va_list va;

    va_start(va, s);
    do {
        write(fd, s, strlen(s));
        s = va_arg(va, const char *);
    } while (s);
    va_end(va);
}

int main(int ac, char **av)
{
    int i, ret, old, new;

    if (ac < 2) {
        errmsg("usage: sysctl name[=value]\n");
        exit(-1);
    }

    for (i = 1; i < ac; i++) {
        char *p = strchr(av[i], '=');
        if (p) {
            *p = '\0';
            new = atoi(p+1);
            ret = sysctl(av[i], 0, &new);
            if (ret) {
                msg(STDOUT_FILENO, "Failed to set option: ", av[i], "\n", 0);
                continue;
            }
        }
        ret = sysctl(av[i], &old, 0);
        if (ret)
            msg(STDERR_FILENO, "Invalid option: ", av[i], "\n", 0);
        else {
            msg(STDOUT_FILENO, av[i], "=", itoa(old), "\n", 0);
        }
    }
    return ret;
}
