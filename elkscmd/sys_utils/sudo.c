/*
 * sudo - execute a command as superuser
 *
 * Usage: sudo <command> [args...]
 *
 * If already root, executes directly.
 * Otherwise prompts for root password and runs command as root.
 */

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct passwd *pwd;
    char *pbuf, salt[3];

    if (argc < 2) {
        fprintf(stderr, "Usage: sudo <command> [args...]\n");
        return 1;
    }

    if (getuid() == 0) {
        execvp(argv[1], &argv[1]);
        fprintf(stderr, "sudo: %s: command not found\n", argv[1]);
        return 1;
    }

    pwd = getpwnam("root");
    if (!pwd) {
        fprintf(stderr, "sudo: no root entry in /etc/passwd\n");
        return 1;
    }

    if (pwd->pw_passwd[0] == '\0') {
        /* no password set for root, just run it */
        setuid(0);
        setgid(0);
        execvp(argv[1], &argv[1]);
        fprintf(stderr, "sudo: %s: command not found\n", argv[1]);
        return 1;
    }

    pbuf = getpass("Password:");
    if (!pbuf) return 1;

    /* check password - root password must be at least 2 chars for salt */
    if (pwd->pw_passwd[0] && pwd->pw_passwd[1]) {
        salt[0] = pwd->pw_passwd[0];
        salt[1] = pwd->pw_passwd[1];
        salt[2] = 0;
        if (!strcmp(crypt(pbuf, salt), pwd->pw_passwd)) {
            setuid(0);
            setgid(0);
            execvp(argv[1], &argv[1]);
            fprintf(stderr, "sudo: %s: command not found\n", argv[1]);
            return 1;
        }
    }

    fprintf(stderr, "Sorry, try again.\n");
    return 1;
}
