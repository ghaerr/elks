#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include "cmd.h"
#include "lib.h"

int
dirname_main(int argc, char * argv[])
{
	char *line;
	
	if (argc == 2) {
		line = dirname(argv[1]);
		write(STDOUT_FILENO,dirname(line),strlen(line));
		write(STDOUT_FILENO,"\n",1);
	}

	return 0;
}
