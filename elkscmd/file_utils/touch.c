#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>


int main(int argc, char **argv)
{
	int i, ncreate = 0;
	struct stat sbuf;
	int er = 0;

	if (argc < 2) {
		write(STDERR_FILENO, "usage: touch file1 [file2] [file3] ...\n", 39);
		exit(1);
	}
	if ((argv[1][0] == '-') && (argv[1][1] == 'c'))
		ncreate = 1;

	for (i = ncreate + 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (stat(argv[i], &sbuf)) {
				if (!ncreate)
					er = close(creat(argv[i], 0666));
			} else
				er = utime(argv[i], NULL);
		}
	}
	exit(er ? 0 : 1);
}
