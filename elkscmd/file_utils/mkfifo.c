#include <unistd.h>
#include <sys/stat.h>


int
main (argc,argv)
	int argc;
	char **argv;
{
	unsigned short newmode;
	int i,er=0;
	
	newmode = 0666 & ~umask(0);
	for(i=1;i<argc;i++)
	{
/* The first line below mith mkfifo is used in the GNU version but there
   is no mkfifo call in elks libc yet */
/*		if (mkfifo (argv[i],newmode)) */
		if (mknod  (argv[i],newmode | S_IFIFO, 0))
		{
			write(STDERR_FILENO,"mkfifo: cannot make fifo ",25);
			write(STDERR_FILENO,argv[i],strlen(argv[i]));
			write(STDERR_FILENO,"\n",1);
			er&=1;
		}
	}
	exit(er);
}
