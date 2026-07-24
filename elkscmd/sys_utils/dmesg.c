#define __KERNEL__
#include <linuxmt/ntty.h>       /* for struct tty */
#undef __KERNEL__

#include <linuxmt/mm.h>
#include <linuxmt/mem.h>
#include <linuxmt/memory.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

struct dmesg_queue {
    unsigned int     len;
    unsigned int     size;
    unsigned int     head;
    unsigned int     tail;
    unsigned char    base[1];
};

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

static int memread(int fd, word_t off, seg_t seg, void *buf, int size)
{
    if (lseek(fd, _MK_LP(seg, off), SEEK_SET) == (off_t)-1)
        return -1;
    return read(fd, buf, size);
}

static void ring_read(int fd, seg_t seg, struct dmesg_queue *q)
{
    unsigned int avail, first, tail;
    unsigned char buf[2048];
    int r;

    avail = q->len;
    if (avail > q->size)
        avail = q->size;

    tail = (q->head - avail) % q->size;

    /* first part: tail to end of ring */
    first = q->size - tail;
    if (first > avail)
        first = avail;

    r = memread(fd, (sizeof(*q) - 1) + tail, seg, buf, first);
    if (r > 0)
        write(STDOUT_FILENO, buf, r);

    /* second part: wrap around from start of ring */
    if (avail > first) {
        r = memread(fd, sizeof(*q) - 1, seg, buf, avail - first);
        if (r > 0)
            write(STDOUT_FILENO, buf, r);
    }
}

int main(void)
{
    int fd;
    seg_t seg;
    struct dmesg_queue q;

    if ((fd = open("/dev/kmem", O_RDONLY|O_ALT)) < 0) {
        errmsg("no dev/kmem\n");
        return 1;
    }

    if (ioctl(fd, MEM_GETDMSG, &seg) < 0 || seg == 0) {
        errmsg("dmesg not configured\n");
        close(fd);
        return 0;
    }

    if (memread(fd, 0, seg, &q, sizeof(q) - 1) != sizeof(q) - 1) {
        errmsg("dmesg: cannot read header\n");
        close(fd);
        return 1;
    }

    if (q.len > 0)
        ring_read(fd, seg, &q);

    close(fd);
    return 0;
}
