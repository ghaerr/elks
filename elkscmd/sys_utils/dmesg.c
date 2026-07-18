#include <linuxmt/types.h>
#include <linuxmt/mem.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define KMSG_BUF_SIZE   2048

static char buf[KMSG_BUF_SIZE];

int main(void)
{
    int fd;
    unsigned int kmsg_seg;
    struct kmsg_read kr;
    int n;

    fd = open("/dev/kmem", O_RDONLY);
    if (fd < 0) {
        write(2, "dmesg: cannot open /dev/kmem\n", 29);
        return 1;
    }

    if (ioctl(fd, MEM_GETKMSG, &kmsg_seg) < 0) {
        write(2, "dmesg: MEM_GETKMSG failed\n", 26);
        close(fd);
        return 1;
    }

    if (kmsg_seg == 0) {
        write(2, "dmesg: kmsg buffer not configured (add dmesg= to /bootopts)\n", 62);
        close(fd);
        return 0;
    }

    /* Read ring buffer atomically via MEM_READKMSG */
    kr.buf    = buf;
    kr.maxlen = KMSG_BUF_SIZE;
    kr.count  = 0;

    if (ioctl(fd, MEM_READKMSG, &kr) < 0) {
        write(2, "dmesg: MEM_READKMSG failed\n", 27);
        close(fd);
        return 1;
    }

    /* Output */
    n = kr.count;
    {
        char *p = buf;
        while (n > 0) {
            int w = write(1, p, n);
            if (w <= 0) break;
            p += w;
            n -= w;
        }
    }

    close(fd);
    return 0;
}
