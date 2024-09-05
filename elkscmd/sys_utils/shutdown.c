/*
 * reboot/shutdown/poweroff
 *
 * Original version from
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mount.h>
#include <linuxmt/limits.h>
#include <arch/system.h>

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
    int i, force, flag, ret;
    char *progname;

    force = (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 'f');
    if ((progname = strrchr(argv[0], '/')) != NULL)
        progname++;
    else progname = argv[0];
    flag = !strcmp(progname, "reboot")? RB_REBOOT :
           !strcmp(progname, "poweroff")? RB_POWEROFF:
           RB_SHUTDOWN;

    sync();
    for (i = NR_SUPER - 1; i >= 0; --i) {
        ret = try_unmount(i);
        if (ret && !force)      /* -f forces reboot even if mount fails */
            return 1;
    }
    sleep(2);
    if (reboot(0, 0, flag)) {
        perror(progname);
        return 1;
    }
    return 0;
}
