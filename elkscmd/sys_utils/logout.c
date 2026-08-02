/*
 * logout - end a user session
 *
 * Usage: logout [username]
 *
 * Without arguments, sends SIGHUP to current process group.
 * With username (root only), finds the user's login shell
 * and sends SIGHUP to its process group.
 */

#define __KERNEL__
#include <linuxmt/ntty.h>
#undef __KERNEL__

#include <linuxmt/mm.h>
#include <linuxmt/mem.h>
#include <linuxmt/sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <sys/ioctl.h>

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

static int memread(int fd, unsigned int off, unsigned int seg, void *buf, int size)
{
    if (lseek(fd, _MK_LP(seg, off), SEEK_SET) == -1)
        return -1;
    return read(fd, buf, size);
}

int main(int argc, char **argv)
{
    int fd;
    unsigned int j, ds, off;
    struct task_struct task_table;
    struct passwd *pwent;
    uid_t target_uid;
    int maxtasks;
    int found;

    if (argc > 2) {
        errmsg("Usage: logout [username]\n");
        return 1;
    }

    /* no args: logout self - send SIGHUP to our process group */
    if (argc == 1) {
        kill(0, SIGHUP);
        return 0;
    }

    if (getuid() != 0) {
        errmsg("logout: must be root\n");
        return 1;
    }

    pwent = getpwnam(argv[1]);
    if (!pwent) {
        errmsg("logout: unknown user\n");
        return 1;
    }
    target_uid = pwent->pw_uid;

    if ((fd = open("/dev/kmem", O_RDONLY|O_ALT)) < 0) {
        errmsg("logout: cannot open /dev/kmem\n");
        return 1;
    }
    if (ioctl(fd, MEM_GETDS, &ds) < 0 ||
        ioctl(fd, MEM_GETMAXTASKS, &maxtasks) < 0 ||
        ioctl(fd, MEM_GETTASK, &off) < 0) {
        errmsg("logout: cannot get kernel info\n");
        close(fd);
        return 1;
    }

    /* find login shell: process where uid matches and pid == pgrp */
    found = 0;
    for (j = 0; j < maxtasks; j++) {
        if (memread(fd, off + j * sizeof(struct task_struct), ds,
                &task_table, sizeof(task_table)) != sizeof(task_table))
            continue;
        if (task_table.state == TASK_UNUSED ||
            task_table.state == TASK_ZOMBIE ||
            task_table.state == TASK_EXITING)
            continue;
        if (task_table.uid == target_uid && task_table.pid == task_table.pgrp) {
            kill(-task_table.pgrp, SIGHUP);
            found = 1;
        }
    }

    close(fd);
    if (!found) {
        errmsg("logout: user not logged in\n");
        return 1;
    }
    return 0;
}
