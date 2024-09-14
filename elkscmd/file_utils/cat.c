/*
 * /bin/cat for ELKS; mark II
 *
 * 1997 MTK, other insignificant people
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

static char readbuf[BUFSIZ];    /* use disk block size for stack limit and efficiency*/

#define TEST    0
#if TEST
void test(void)
{
    printf("#04X: '%#04X'\n", 0x2ab);
    printf("04X: '%04X'\n", 0x2ab);
    printf("04x: '%04x'\n", 0x2ab);
    printf(" 4x: '%4x'\n", 0x2ab);
    printf("04d: '%04d'\n", 0x200);
    printf(" 4d: '%4d'\n", 0x200);
    printf("05d: '%05d'\n", -20);
    printf(" 5d: '%5d'\n", -20);
    printf("+5d: '%5d'\n", -20);
    printf("+5d: '%5d'\n", 20);
    printf(" ld: '%ld'\n", -123456789L);
    printf(" lx: '%lx'\n", 0x87654321L);
    printf(" lo: '%lo'\n", 0xFFFFFFFFL);
    printf("  s: '%s'\n", "thisisatest");
    printf(" 6s: '%6s'\n", "thisisatest");
    printf("20s: '%20s'\n", "thisisatest");
}
#endif

static int copyfd(int fd)
{
	int n;

	while ((n = read(fd, readbuf, sizeof(readbuf))) > 0) {
		if (write(STDOUT_FILENO, readbuf, n) != n)
			return 1;
    }
	return n < 0? 1: 0;
}

int main(int argc, char **argv)
{
	int i, fd;

#if TEST
    test();
    exit(0);
#endif
	if (argc <= 1) {
		if (copyfd(STDIN_FILENO)) {
			perror("stdin");
			return 1;
		}
	} else {
		for (i = 1; i < argc; i++) {
			errno = 0;
			if (argv[i][0] == '-' && argv[i][1] == '\0')
				fd = STDIN_FILENO;
			else if ((fd = open(argv[i], O_RDONLY)) < 0) {
				perror(argv[i]);
				return 1;
			}
			if (copyfd(fd)) {
				perror(argv[i]);
				close(fd);
				return 1;
			}
			if (fd != STDIN_FILENO)
				close(fd);
		}
	}
	return 0;
}
