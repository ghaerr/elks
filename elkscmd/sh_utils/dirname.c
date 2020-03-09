#include <unistd.h>
#include <string.h>

void
strip_trailing_slashes (path)
char *path;
{
	int last;

	last = strlen (path) - 1;
	while (last > 0 && path[last] == '/')
		path[last--] = '\0';
}


void
main (argc, argv)
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
}
