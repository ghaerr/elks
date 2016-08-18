#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *basename(char *name)
{
	char *base;
	
	base = strrchr(name, '/');
	return base ? base + 1 : name;
}
                                

int main(int argc, char **argv)
{
	int i;	/*, recurse = 0, interact =0 */
	struct stat sbuf;

	if (argc < 2) goto usage;

/*	if (((argv[1][0] == '-') && (argv[1][1] == 'r')) || ((argv[2][0] == '-') && (argv[2][1] == 'r'))) 
		recurse = 1;
	
        if (((argv[1][0] == '-') && (argv[1][1] == 'i')) || ((argv[2][0] == '-') && (argv[2][1] == 'i')))
		interact = 1;        
 */	
	for (i = /*recurse+interact+*/1; i < argc; i++) {
		if (argv[i][0] != '-') {	
			if (!lstat(argv[i],&sbuf)) {
				if (unlink(argv[i])) {
					fprintf(stderr,"rm: could not remove %s\n", argv[i]);
				}
			}
		}
	}
	exit(0);

usage:
	fprintf(stderr, "usage: %s file1 [file2] ...\n", argv[0]);
	exit(1);
}
