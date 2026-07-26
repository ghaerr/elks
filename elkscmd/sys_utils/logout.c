#define __KERNEL__
#include <linuxmt/ntty.h>
#undef __KERNEL__

#include <linuxmt/mm.h>
#include <linuxmt/mem.h>
#include <linuxmt/sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>

static int memread(int fd, word_t off, word_t seg, void *buf, int size)
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
    uid_t target_uid = (uid_t)-1;
    int found = 0;
    int maxtasks;

    if (argc > 2) {
        fprintf(stderr, "Usage: logout [username]\n");
        return 1;
    }

    if (argc == 2) {
        if (getuid() != 0) {
            fprintf(stderr, "logout: only root can logout other users\n");
            return 1;
        }
        pwent = getpwnam(argv[1]);
        if (!pwent) {
            fprintf(stderr, "logout: unknown user '%s'\n", argv[1]);
            return 1;
        }
        target_uid = pwent->pw_uid;
    }

    if ((fd = open("/dev/kmem", O_RDONLY|O_ALT)) < 0) {
        fprintf(stderr, "logout: cannot open /dev/kmem\n");
        return 1;
    }
    if (ioctl(fd, MEM_GETDS, &ds) < 0 ||
        ioctl(fd, MEM_GETMAXTASKS, &maxtasks) < 0) {
        fprintf(stderr, "logout: cannot get kernel info\n");
        close(fd);
        return 1;
    }
    if (ioctl(fd, MEM_GETTASK, &off) < 0) {
        fprintf(stderr, "logout: cannot get task table\n");
        close(fd);
        return 1;
    }

    for (j = 0; j < maxtasks; j++) {
        if (memread(fd, off + j*sizeof(struct task_struct), ds,
                &task_table, sizeof(task_table)) != sizeof(task_table))
            continue;

        if (task_table.state == TASK_UNUSED ||
            task_table.state == TASK_ZOMBIE ||
            task_table.state == TASK_EXITING)
            continue;

        if (target_uid == (uid_t)-1) {
            /* logout self: find our own UID, kill all our TTY processes */
            if (task_table.uid == getuid() && task_table.tty != 0) {
                kill(task_table.pid, SIGHUP);
                found++;
            }
        } else {
            /* logout specific user */
            if (task_table.uid == target_uid && task_table.tty != 0) {
                kill(task_table.pid, SIGHUP);
                found++;
            }
        }
    }

    close(fd);

    if (target_uid != (uid_t)-1 && !found)
        fprintf(stderr, "logout: no active sessions for '%s'\n", argv[1]);

    return 0;
}
