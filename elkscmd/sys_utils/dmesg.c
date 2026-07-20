#include <linuxmt/types.h>
#include <linuxmt/mem.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define DMSG_BUF_SIZE   2048

#define DMESG_BUFSIZ    2048
static char localbuf[DMESG_BUFSIZ];

void dmesg_write(unsigned char __far *farbuf, unsigned int count)
{
    unsigned int n;

    while (count) {
        n = MIN(count, DMESG_BUFSIZ);
        fmemcpy(localbuf, farbuf, n);
        write(STDOUT_FILENO, localbuf, n);
        count -= n;
    }
}

void dmesg_read(void)
{
    unsigned int tail, n;
    struct dmesg_queue __far *q = _MK_FP(dmesg_seg, 0);

    if (!q->len)
        return;
    tail = (q->head - q->len) % q->size;    /* start read pointer */
    n = q->size - tail;                     /* bytes to end of buffer */
    dmesg_write(&q->base[tail], n);
    n = q->len - n;                         /* bytes from start of buffer */
    dmesg_write(&q->base[0], n);
}
//static char buf[DMSG_BUF_SIZE];

int main(void)
{
   /* int fd;
    unsigned int dmsg_seg;
    struct dmsg_read kr;
    int n;

    fd = open("/dev/kmem", O_RDONLY);
    if (fd < 0) {
        write(2, "dmesg: cannot open /dev/kmem\n", 29);
        return 1;
    }

    if (ioctl(fd, MEM_GETKMSG, &kmsg_seg) < 0) {
        write(2, "dmesg: kernel message log not enabled\n", 38);
        close(fd);
        return 1;
    }

    if (kmsg_seg == 0) {
        write(2, "dmesg: kmsg buffer not configured (add dmesg= to /bootopts)\n", 60);
        close(fd);
        return 0;
    }

    /* Read ring buffer atomically via MEM_READKMSG */
/*    kr.buf    = buf;
    kr.maxlen = KMSG_BUF_SIZE;
    kr.count  = 0;

    if (ioctl(fd, MEM_READKMSG, &kr) < 0) {
        write(2, "dmesg: MEM_READKMSG failed\n", 27);
        close(fd);
        return 1;
    }

    /* Output */
  /*  n = kr.count;
    {
        char *p = buf;
        while (n > 0) {
            int w = write(1, p, n);
            if (w <= 0) break;
            p += w;
            n -= w;
        }
    }

    close(fd);*/
    dmesg_read();
    return 0;
}
