#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


int
main (argc,argv)
	int argc;
	char **argv;
{
	int i, ncreate = 0;
	struct stat sbuf;
	int fd,er;
	
	if ((argv[1][0] == '-') && (argv[1][1] == 'c'))
		ncreate = 1;
	
	for(i=ncreate+1;i<argc;i++) {
		if (argv[i][0] != '-') {	
			if (stat(argv[i],&sbuf)) {
				if (!ncreate)
					er = close(creat(argv[i], 0666));				
			} else
				er = utime(argv[i],NULL);
		}
	}
}
