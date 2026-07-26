/*
 * sudo - execute a command as superuser
 *
 * Usage: sudo <command> [args...]
 *
 * Requires caller to be in "wheel" group and know root password.
 * If already root, executes directly.
 */

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int in_wheel_group(uid_t uid)
{
    struct group *grp;
    char **mem;

    grp = getgrnam("wheel");
    if (!grp) return 0;

    for (mem = grp->gr_mem; *mem; mem++) {
        struct passwd *pw = getpwnam(*mem);
        if (pw && pw->pw_uid == uid)
            return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct passwd *pwd;
    char *pbuf, salt[3];
    uid_t uid;

    if (argc < 2) {
        fprintf(stderr, "Usage: sudo <command> [args...]\n");
        return 1;
    }

    uid = getuid();

    if (uid == 0) {
        execvp(argv[1], &argv[1]);
        fprintf(stderr, "sudo: %s: command not found\n", argv[1]);
        return 1;
    }

    if (!in_wheel_group(uid)) {
        fprintf(stderr, "sudo: user not in wheel group\n");
        return 1;
    }

    pwd = getpwnam("root");
    if (!pwd) {
        fprintf(stderr, "sudo: no root entry in /etc/passwd\n");
        return 1;
    }

    if (pwd->pw_passwd[0] == '\0') {
        setuid(0);
        setgid(0);
        execvp(argv[1], &argv[1]);
        fprintf(stderr, "sudo: %s: command not found\n", argv[1]);
        return 1;
    }

    pbuf = getpass("Password:");
    if (!pbuf) return 1;

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
