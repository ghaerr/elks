#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linuxmt/mem.h>
#include <linuxmt/kernel.h>
#include <linuxmt/segment.h>

#define MIN(a,b)    ((a) < (b)? (a): (b))
#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

static unsigned char buf[2048];

static int memread(int fd, unsigned int off, unsigned int seg, void *buf, int size)
{
    if (lseek(fd, _MK_LP(seg, off), SEEK_SET) == (off_t)-1)
        return -1;
    return read(fd, buf, size);
}

static void ring_display(int fd, unsigned int seg, unsigned int start, unsigned int count)
{
    unsigned int n;

    while (count) {
        n = MIN(count, sizeof(buf));
        n = memread(fd, sizeof(struct dmesg_queue) + start, seg, buf, n);
        if (n <= 0)
            break;
       write(STDOUT_FILENO, buf, n);
       start += n;
       count -= n;
    }
}

static void ring_read(int fd, unsigned int seg, struct dmesg_queue *q)
{
    unsigned int avail, first, tail;

    if (!q->len)
        return;

    avail = q->len;
    if (avail > q->size)
        avail = q->size;

    tail = (avail == q->size)? q->head: 0;

    /* first part: tail to end of ring */
    first = q->size - tail;
    if (first > avail)
        first = avail;

    ring_display(fd, seg, tail, first);

    /* second part: wrap around from start of ring */
    if (avail > first)
        ring_display(fd, seg, 0, avail - first);
}

int main(void)
{
    int fd;
    unsigned int seg;
    struct dmesg_queue q;

    if ((fd = open("/dev/kmem", O_RDONLY|O_ALT)) < 0) {
        errmsg("no /dev/kmem\n");
        return 1;
    }

    if (ioctl(fd, MEM_GETDMSG, &seg) < 0 || seg == 0) {
        errmsg("dmesg not configured\n");
        close(fd);
        return 0;
    }

    if (memread(fd, 0, seg, &q, sizeof(q)) != sizeof(q)) {
        errmsg("dmesg: cannot read queue\n");
        close(fd);
        return 1;
    }

    ring_read(fd, seg, &q);

    close(fd);
    return 0;
}
