#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "futils.h"

static unsigned short newmode;

static int remove_dir(char *name, int f)
{
	int er, era=2;
	char *line;
	
	while (((er = rmdir(name)) == 0) && ((line = rindex(name,'/')) != NULL) && f) {
		while ((line > name) && (*line == '/'))
			--line;
		line[1] = 0;
		era=0;
	}
	return(er && era);
}
	

int main(int argc, char **argv)
{
	int i, parent = 0, er = 0;
	
	if (argc < 2) goto usage;

	if ((argv[1][0] == '-') && (argv[1][1] == 'p'))	
		parent = 1;
	
	newmode = 0666 & ~umask(0);

	for (i = parent + 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			while (argv[i][strlen(argv[i])-1] == '/')
				argv[i][strlen(argv[i])-1] = '\0';
			if (remove_dir(argv[i],parent)) {
				errstr(argv[i]);
				errmsg(": cannot remove directory\n");
				er = 1;
			}
		} else goto usage;
	}
	return er;

usage:
	errmsg("usage: rmdir directory [...]\n");
	return 1;
}
