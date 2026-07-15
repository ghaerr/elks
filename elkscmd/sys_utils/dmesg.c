#include <linuxmt/types.h>
#include <linuxmt/mem.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
 * Ring buffer header layout in far memory (all offsets from seg:0):
 *   0: head (unsigned int)  - write position
 *   2: tail (unsigned int)  - read position
 *   4: count (unsigned int) - bytes available
 *   6: size  (unsigned int) - data capacity
 *   8: data[0..size-1]     - circular buffer
 */
#define KMSG_HDR_SIZE   8

int main(void)
{
    int fd;
    unsigned int kmsg_seg;
    unsigned int hdr[4];        /* head, tail, count, size */
    unsigned int count, tail, size;
    unsigned long pos;
    char buf[256];
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
        return 1;
    }

    /* Use O_ALT far pointer mode: f_pos = (seg << 16) | offset */
    close(fd);
    fd = open("/dev/kmem", O_RDONLY | O_ALT);
    if (fd < 0) {
        write(2, "dmesg: cannot open /dev/kmem (O_ALT)\n", 38);
        return 1;
    }

    /* seek to ring buffer header: far pointer = kmsg_seg:0 */
    pos = (unsigned long)kmsg_seg << 16;
    lseek(fd, pos, SEEK_SET);

    /* read header: head, tail, count, size */
    if (read(fd, (char *)hdr, KMSG_HDR_SIZE) != KMSG_HDR_SIZE) {
        write(2, "dmesg: cannot read header\n", 26);
        close(fd);
        return 1;
    }

    count = hdr[2];     /* count */
    tail  = hdr[1];     /* tail */
    size  = hdr[3];     /* size */

    if (count == 0) {
        close(fd);
        return 0;
    }

    /* seek to data area: kmsg_seg:(KMSG_HDR_SIZE + tail) */
    pos = ((unsigned long)kmsg_seg << 16) + KMSG_HDR_SIZE + tail;
    lseek(fd, pos, SEEK_SET);

    /* read the linear portion (tail to end of buffer) */
    n = size - tail;
    if (n > count)
        n = count;

    while (n > 0) {
        int toread = n > (int)sizeof(buf) ? (int)sizeof(buf) : n;
        int r = read(fd, buf, toread);
        if (r <= 0) break;
        write(1, buf, r);
        n -= r;
    }

    /* if wrapped, read from beginning of data area */
    if (count > (size - tail)) {
        pos = ((unsigned long)kmsg_seg << 16) + KMSG_HDR_SIZE;
        lseek(fd, pos, SEEK_SET);
        n = count - (size - tail);
        while (n > 0) {
            int toread = n > (int)sizeof(buf) ? (int)sizeof(buf) : n;
            int r = read(fd, buf, toread);
            if (r <= 0) break;
            write(1, buf, r);
            n -= r;
        }
    }

    close(fd);
    return 0;
}
