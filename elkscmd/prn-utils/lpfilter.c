/*
 * lp printing filter
 * use -pcl switch if you have PCL or compatible printer
 */
 
#include <stdio.h>

#define BUFFER_SIZE 256
#define DEBUG

#undef DEBUG

int main(argc, argv)
int argc;
char ** argv;
{
	char * buffer;
	char pos;

	buffer = calloc(BUFFER_SIZE, 1);
	if (!buffer) {
		fprintf(stderr, "%s: unable to allocate %d bytes of memory\n", argv[0], BUFFER_SIZE);
		return -1;
	}

	if (!strcmp(argv[1], "-pcl"))
	{
#ifdef DEBUG
		fprintf(stderr, "Invoking PCL filter ..\n");
#endif	
		/* ]E]&k2g](0N --> reset printer, set CR = CRLF, FF = CRFF, set latin 1 character set */
		fprintf(stdout, "\033E\033&k2g\033(0N");
		/* feed printer with unmodified data */
		while (!feof(stdin)) {
			fgets(buffer, BUFFER_SIZE, stdin);
			fputs(buffer, stdout);
			}

		/* ]E --> reset printer */
		fprintf(stdout, "\033E");

	}
	else {
#ifdef DEBUG
		fprintf(stderr, "Invoking ESC/P filter ..\n");
#endif
		/* set USA charset */
		fprintf(stdout, "\033R0");
		while (!feof(stdin)) {
			/* we need extra sapce at the end of buffer should the input string be too long */
			fgets(buffer, BUFFER_SIZE - 3, stdin);
			/* substitute LF#0/FF#0 sequence for CRLF#0/CRFF#0 */
			for (pos = 0; pos <= BUFFER_SIZE - 3; pos++) {
				if ((buffer[pos] == '\012') || (buffer[pos] == '\014')) {
#ifdef DEBUG
					fprintf(stderr, "LF or FF w/out CR\n");
#endif					
					buffer[pos + 1] = buffer[pos]; /* FF or LF */
					buffer[pos + 0] = '\015'; /* CR */
					buffer[pos + 2] = '\000'; /* terminate string */
					break;
					}
				}
			fputs(buffer, stdout);
			}	
	}
	free(buffer);

	return 0;
}


