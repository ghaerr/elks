#include <unistd.h>
#include <string.h>

void
main (argc, argv)
	int argc;
	char **argv;
{
	if (argc == 1)
		while (1)
			write(STDOUT_FILENO,"y\n",2);
			
	while (1) {
		int i;
		for (i = 1; i < argc; i++) {
			write(STDOUT_FILENO,argv[i], strlen(argv[i]));
			write(STDOUT_FILENO,i == argc - 1 ? "\n" : " ",1);
		}
	}
	                                                          
}
