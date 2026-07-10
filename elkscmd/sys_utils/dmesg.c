#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    char buf[256];
    int n;
    int fd = open("/dev/kmsg", O_RDONLY);

    if (fd < 0) {
        write(2, "dmesg: cannot open /dev/kmsg\n", 29);
        return 1;
    }

    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, n);

    close(fd);
    return 0;
}
