#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

unsigned short newmode;

int
make_dir(name,f)
	char *name;
	int f;
{
	char iname[256];
	char *line;
	
	strcpy(iname, name);
	if (((line = rindex(iname,'/')) != NULL) && f) {
		while ((line > iname) && (*line == '/'))
			--line;
		line[1] = 0;
		make_dir(iname,1);
	}
	if (mkdir(name, newmode) && !f)
		return(1);
	else
		return(0);

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
			if (argv[i][strlen(argv[i])-1] == '/')
				argv[i][strlen(argv[i])-1] = '\0';

			if (make_dir(argv[i],parent)) {
				write(STDERR_FILENO,"mkdir: cannot create directory ",31);
				write(STDERR_FILENO,argv[i],strlen(argv[i]));
				write(STDERR_FILENO,"\n",1);
				er = 1;
			}
		} else {
			write(STDERR_FILENO,"mkdir: usage error.\n",20);
			exit(1);
		}
	}
	exit(er);
}
