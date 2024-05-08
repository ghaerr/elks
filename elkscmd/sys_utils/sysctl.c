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

static int digit(char c, int base)
{
    int d;
    if (c <= '9') {
        d = c - '0';
    } else if (c < 'A') {   /* NEATLIBC bugfix */
        return -1;
    } else if (c <= 'Z') {
        d = 10 + c - 'A';
    } else {
        d = 10 + c - 'a';
    }
    return d < base ? d : -1;
}

static int strtoi(const char *s)
{
    int num;
    int dig;
    int base;
    int sgn = 1;

    if (*s == '-')
        sgn = ',' - *s++;
    if (*s == '0') {
        if (s[1] == 'x' || s[1] == 'X') {
            base = 16;
            s += 2;
        } else
            base = 8;
    } else {
        base = 10;
    }
    for (num = 0; (dig = digit(*s, base)) >= 0; s++)
        num = num*base + dig;
    num *= sgn;
    return num;
}

static void msg(int fd, const char *s, ...)
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
            new = strtoi(p+1);
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
