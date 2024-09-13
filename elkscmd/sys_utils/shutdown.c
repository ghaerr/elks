/*
 * reboot/shutdown/poweroff
 *
 * Usage: {shutdown,reboot,poweroff} [-s][-r][-p][-f]
 *
 * Completely rewritten from original version of
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <linuxmt/limits.h>
#include <arch/system.h>

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

static int f_flag;      /* continue even if unmounts fail */

static void usage(void)
{
    errmsg("Usage: shutdown [-s][-r][-p][-f]\n");
    exit(1);
}

static int try_unmount(dev_t dev)
{
    struct statfs statfs;

    if (ustatfs(dev, &statfs, UF_NOFREESPACE) < 0) {
        return 0;
    }
    if (umount(statfs.f_mntonname)) {
        perror("umount");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    char *p, *progname;
    int i, how;

    if ((progname = strrchr(argv[0], '/')) != NULL)
        progname++;
    else progname = argv[0];
    how = !strcmp(progname, "reboot")? RB_REBOOT :
           !strcmp(progname, "poweroff")? RB_POWEROFF:
           RB_SHUTDOWN;

	while (argv[1] && argv[1][0] == '-') {
		for (p = &argv[1][1]; *p; p++) {
            switch (*p) {
            case 's':
                how = RB_SHUTDOWN; break;
            case 'r':
                how = RB_REBOOT; break;
            case 'p':
                how = RB_POWEROFF; break;
            case 'f':
                f_flag = 1; break;
            default:
		        usage();
            }
        }
		argv++;
		argc--;
	}
    if (argc > 1) usage();

    sync();
    for (i = NR_SUPER - 1; i >= 0; --i) {
        if (try_unmount(i) && !f_flag)
            return 1;               /* stop on failed unmount unless -f specified */
    }
    sleep(2);
    if (reboot(0, 0, how)) {
        perror(progname);
        return 1;
    }
    return 0;
}
