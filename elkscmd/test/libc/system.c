#include "testlib.h"

#include <dirent.h>
#include <errno.h>
#include <linuxmt/fs.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <unistd.h>

TEST_CASE(system_ioctl)
{
	/* TODO does not fail! */
	ASSERT_SYS(ioctl(999, 0), -1, EBADF);
}

TEST_CASE(system_umount)
{
	/* error cases */
	ASSERT_SYS(umount("doesnotexist"), -1, ENOENT);

	/* TODO external fixture */
}

TEST_CASE(system_ustatfs)
{
	struct statfs buf;

	/* TODO is statfs,fstatfs implemented? */
	/*ASSERT_SYS(statfs("/", *buf), 0, 0);*/

	ASSERT_SYS(ustatfs(999, &buf, 0), -1, EINVAL);
}

TEST_CASE(system_reboot)
{
	if (geteuid() == 0) {
		ASSERT_SYS(reboot(1, 2, 3), -1, EINVAL);
	} else {
		ASSERT_SYS(reboot(1, 2, 3), -1, EPERM);
	}
}

TEST_CASE(system_sleep)
{
	EXPECT_EQ(sleep(0), 0);

	/* TODO interrupt with signal */
}

TEST_CASE(system_dirent)
{
    DIR *d;
    struct dirent *de;

    d = opendir("doesnotexist");
    EXPECT_EQ(errno, ENOENT);
    ASSERT_EQ_P(d, NULL);

    d = opendir("/");
    ASSERT_NE_P(d, NULL);
    for (de = NULL; de; ) {
        errno = 0;
        de = readdir(d);
        if (de == NULL) {
            ASSERT_NE(errno, 0);
            break;
        }
    }
    /* TODO rewinddir, seekdir, telldir */
    ASSERT_SYS(closedir(d), 0, 0);
}

TEST_CASE(system_scandir)
{
	/* TODO */
}

TEST_CASE(system_fcntl)
{
	/* TODO creat fcntl open */
}

TEST_CASE(system_signal)
{
	/* TODO signal, kill */
}

TEST_CASE(system_fifo)
{
}
