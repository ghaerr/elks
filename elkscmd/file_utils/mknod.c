#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#define makedev(maj, min)  (((maj) << 8) | (min))

int
main (argc,argv)
	int argc;
	char **argv;
{
	unsigned short newmode;
	unsigned short filetype;
	int major,minor;
	
	newmode = 0666 & ~umask(0);
	
	if (argc == 5) {
		switch(argv[2][0]) {
		case 'b':
			filetype = S_IFBLK;
			break;
		case 'c':
		case 'u':
			filetype = S_IFCHR;
			break;
		default:
			write(STDERR_FILENO,"mknod: usage error\n",19);
			exit(1);
		}
		major = (int)strtol(argv[3],NULL,0);
		minor = (int)strtol(argv[4],NULL,0);
		
		if ( errno != ERANGE )
			if (mknod (argv[1], newmode | filetype, makedev(major, minor))) {
				write(STDERR_FILENO,"mknod: cannot make device ",27);
				write(STDERR_FILENO,argv[1],strlen(argv[1]));
				write(STDERR_FILENO,"\n",1);
				exit(1);
			}
	} else if ((argc == 3) && (argv[2][0] == 'p')) {
	
/* The second line mith mkfifo is used in the GNU version but there
   is no mkfifo call in elks libc yet */
		if (mknod (argv[1],newmode | S_IFIFO, 0))
/*		if (mkfifo (argv[1],newmode)) */
		{
			write(STDERR_FILENO,"mknod: cannot make fifo ",25);
			write(STDERR_FILENO,argv[1],strlen(argv[1]));
			write(STDERR_FILENO,"\n",1);
			exit(1);
		}

	} else {
	
		write(STDERR_FILENO,"mknod: usage error\n",19);
	}
	exit(0);
}
