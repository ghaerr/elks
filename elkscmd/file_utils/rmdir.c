#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "futils.h"

static int remove_dir(char *name, int f)
{
	int er, era=2;
	char *line;
	
	while (((er = rmdir(name)) == 0) && ((line = strrchr(name,'/')) != NULL) && f) {
		while ((line > name) && (*line == '/'))
			--line;
		line[1] = 0;
		era=0;
	}
	return(er && era);
}
	

int main(int argc, char **argv)
{
	int i, parent = 0, force = 0, er = 0;
	
	if (argc < 2) goto usage;

        while (argv[1][0] == '-') {
                switch (argv[1][1]) {
                case 'p': parent = 1; break;
                case 'f': force = 1; break;
                default: goto usage;
                }
                argv++;
                argc--;
        }
	

	for (i = parent + 1; i < argc; i++) {
			while (argv[i][strlen(argv[i])-1] == '/')
				argv[i][strlen(argv[i])-1] = '\0';
			if (remove_dir(argv[i],parent)) {
				perror(argv[i]);
				er = 1;
			}
	}
	return force? 0: er;

usage:
	errmsg("usage: rmdir [-pf] directory [...]\n");
	return 1;
}
