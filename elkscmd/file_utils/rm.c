#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *
basename (name)
	char *name;
{
	char *base;
	
	base = rindex (name, '/');
	return base ? base + 1 : name;
}
                                

int
main (argc,argv)
	int argc;
	char **argv;
{
	int i/*, recurse = 0, interact =0*/;
	struct stat sbuf;
	int fd,er;
	
/*	if (((argv[1][0] == '-') && (argv[1][1] == 'r')) || ((argv[2][0] == '-') && (argv[2][1] == 'r'))) 
		recurse = 1;
	
        if (((argv[1][0] == '-') && (argv[1][1] == 'i')) || ((argv[2][0] == '-') && (argv[2][1] == 'i')))
		interact = 1;        
 */	
	for(i=/*recurse+interact+*/1;i<argc;i++) {
		if (argv[i][0] != '-') {	
			if (!lstat(argv[i],&sbuf)) {
				if (unlink(argv[i])) {
					write(STDERR_FILENO,"rm: could not remove ",21);
					write(STDERR_FILENO,argv[i],strlen(argv[i]));
					write(STDERR_FILENO,"\n",1);
				}
			}
		}
	}
}
