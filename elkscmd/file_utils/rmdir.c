#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

unsigned short newmode;

int
remove_dir(name,f)
	char *name;
	int f;
{
	int er,era=2;
	char *line;
	
	while (((er = rmdir(name)) == 0) && ((line = rindex(name,'/')) != NULL) && f) {
		while ((line > name) && (*line == '/'))
			--line;
		line[1] = 0;
		era=0;
	}
	return(er && era);
}
	

int
main (argc,argv)
	int argc;
	char **argv;
{
	int i, parent = 0, er = 0;
	
	if ((argv[1][0] == '-') && (argv[1][1] == 'p'))	
		parent = 1;
	
	newmode = 0666 & ~umask(0);

	for(i=parent+1;i<argc;i++) {
		if (argv[i][0] != '-') {
			while (argv[i][strlen(argv[i])-1] == '/')
				argv[i][strlen(argv[i])-1] = '\0';
			if (remove_dir(argv[i],parent)) {
				write(STDERR_FILENO,"rmdir: cannot remove directory ",31);
				write(STDERR_FILENO,argv[i],strlen(argv[i]));
				write(STDERR_FILENO,"\n",1);
				er = 1;
			}
		} else {
			write(STDERR_FILENO,"rmdir: usage error.\n",20);
			exit(1);
		}
	}
	exit(er);
}
