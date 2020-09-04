#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>


int main(int argc, char **argv)
{
	int i, ncreate = 0;
	int err = 0;
	struct stat sbuf;

	if (argc < 2) {
		write(STDERR_FILENO, "usage: touch file1 [file2] [file3] ...\n", 39);
		return 1;
	}
	if ((argv[1][0] == '-') && (argv[1][1] == 'c'))
		ncreate = 1;

	for (i = ncreate + 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (stat(argv[i], &sbuf)) {
				if (!ncreate) {
					int fd = creat(argv[i], 0666);
					if (fd < 0)
						perror("touch"), err = 1;
					else close(fd);
				}
			} else
				err |= utime(argv[i], NULL);
		}
	}
	return (err ? 1 : 0);
}
