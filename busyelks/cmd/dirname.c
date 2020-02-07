#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lib.h"

void
dirname_main (argc, argv)
	int argc;
	char **argv;
{
	char *line;
	
	if (argc == 2) {
		strip_trailing_slashes(argv[1]);
		line = rindex (argv[1],'/');
		if (line == NULL) {
			argv[1][0]='.';
			argv[1][1]=0;
		} else {
			while (line > argv[1] && *line == '/')
				--line;
			line[1] = 0;
		}                    
		
		write(STDOUT_FILENO,argv[1],strlen(argv[1]));
		write(STDOUT_FILENO,"\n",1);
	}
	exit(0);
}
