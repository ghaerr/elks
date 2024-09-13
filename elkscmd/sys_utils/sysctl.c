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
    int i, ret, val;
    char name[CTL_MAXNAMESZ];

    if (ac < 2) {
        errmsg("usage: sysctl [-a] [name[=value]]\n");
        exit(-1);
    }

    if (av[1][0] == '-' && av[1][1] == 'a') {
        for (i = 0;; i++) {
            ret = sysctl(CTL_LIST+i, name, 0);
            if (ret) exit(0);
            ret = sysctl(CTL_GET, name, &val);
            msg(STDOUT_FILENO, name, "=", itoa(val), "\n", 0);
        }
    }

    for (i = 1; i < ac; i++) {
        char *p = strchr(av[i], '=');
        if (p) {
            *p = '\0';
            val = strtoi(p+1);
            ret = sysctl(CTL_SET, av[i], &val);
            if (ret) {
                msg(STDOUT_FILENO, "Unknown option: ", av[i], "\n", 0);
                continue;
            }
        }
        ret = sysctl(CTL_GET, av[i], &val);
        if (ret)
            msg(STDERR_FILENO, "Invalid option: ", av[i], "\n", 0);
        else {
            msg(STDOUT_FILENO, av[i], "=", itoa(val), "\n", 0);
        }
    }
    return ret;
}
