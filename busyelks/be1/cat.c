/*
 * /bin/cat for ELKS; mark II
 *
 * 1997 MTK, other insignificant people
 */

#define __USE_BSD
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define CAT_BUF_SIZE 4096

int cat_read_size = 1;
char colon[2] = { ':', ' ' };
char nl = '\n';

void dumpfile(fd)
int fd;
{
	int nred;
	char readbuf[CAT_BUF_SIZE];

	while ((nred=read(fd,readbuf,cat_read_size)) > 0) {
		write(STDOUT_FILENO,readbuf,nred);
	}
}

int cat_main(argc,argv)
int argc;
char **argv;
{
	int i, fd;

	if(argc <= 1) {
		dumpfile(STDIN_FILENO);
	} else {
		for(i=1;i<argc;i++) {
			fd = open(argv[i], O_RDONLY);
			if(fd == -1) {
				write(STDERR_FILENO, argv[0], strlen(argv[0]));
				write(STDERR_FILENO, colon, 2);
				write(STDERR_FILENO, argv[i], strlen(argv[i]));
				write(STDERR_FILENO, colon, 2);
				write(STDERR_FILENO, sys_errlist[errno], strlen(sys_errlist[errno]));
				write(STDERR_FILENO, &nl, 1);
			} else {
				dumpfile(fd);
				close(fd);
			}
		}
	}
	exit(0);
}
