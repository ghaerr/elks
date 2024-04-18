/*
 * A front-end to the mread/mwrite commands.
 *
 * Emmet P. Gray			US Army, HQ III Corps & Fort Hood
 * ...!uunet!uiucuxc!fthood!egray	Attn: AFZF-DE-ENV
 * 					Directorate of Engineering & Housing
 * 					Environmental Management Office
 * 					Fort Hood, TX 76544-5057
 */

#include <stdio.h>

#define NONE	0
#define MREAD	1
#define MWRITE	2

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int i, oops, msdos_args, unix_args, destination;
	char **nargv, *strcpy();
	void exit();
					/* get command line options */
	msdos_args = 0;
	unix_args = 0;
	oops = 0;
	while ((i = getopt(argc, argv, "tnvm")) != EOF) {
		switch(i) {
			case 't':
			case 'n':
			case 'v':
			case 'm':
				break;
			default:
				oops = 1;
				break;
		}
	}

	if (oops || (argc - optind) < 2) {
		fprintf(stderr, "Usage: mcopy [-tnvm] a:msdosfile unixfile\n");
		fprintf(stderr, "       mcopy [-tnvm] a:msdosfile [a:msdosfiles...] unixdirectory\n");
		fprintf(stderr, "       mcopy [-tnm] unixfile a:msdosfile\n");
		fprintf(stderr, "       mcopy [-tnm] unixfile [unixfiles...] a:msdosdirectory\n");
		exit(1);
	}
					/* what is the destination drive? */
	destination = NONE;
					/* first file */
	if (argv[optind][1] == ':') {
		if (argv[optind][0] == 'a' || argv[optind][0] == 'A')
			destination = MREAD;
	}
					/* last file */
	if (destination == NONE && argv[argc-1][1] == ':') {
		if (argv[argc-1][0] == 'a' || argv[argc-1][0] == 'A')
			destination = MWRITE;
	}
	if (destination == NONE) {
		fprintf(stderr, "mcopy: no 'a:' designation specified\n");
		exit(1);
	}
					/* strip out the fake "drive code" */
	for (i=optind; i<argc; i++) {
		if (argv[i][1] == ':') {
			switch(argv[i][0]) {
				case 'a':
				case 'A':
					if (argv[i][2] == '\0')
						strcpy(argv[i], ".");
					else
						strcpy(argv[i], &argv[i][2]);
					msdos_args++;
					break;
				case 'c':
				case 'C':
					fprintf(stderr, "mcopy: 'c:' is not used to designate Unix files\n");
					exit(1);
				default:
					fprintf(stderr, "mcopy: unknown drive '%c:'\n", argv[i][0]);
					exit(1);
			}
			continue;
		}
					/* if no drive code, its a unix file */
		unix_args++;
	}
					/* sanity checking */
	if (!msdos_args || !unix_args) {
		fprintf(stderr, "mcopy: unresloved destination\n");
		exit(1);
	}
	if ((destination == MWRITE && msdos_args > 1) || (destination == MREAD && unix_args > 1)) {
		fprintf(stderr, "mcopy: duplicate destination files\n");
		exit(1);
	}
	/*
	 * Copy the *argv[] array in case your Unix doesn't end the array
	 * with a null when it passes it to main()
	 */
	nargv = (char **) malloc((argc+1) * sizeof(*argv));
	nargv[argc] = NULL;
	for (;--argc>=0;)
		nargv[argc] = argv[argc];

	if (destination == MWRITE) {
		nargv[0] = "mwrite";
		execvp("mwrite", nargv);
		perror("execvp: mwrite") ;
	}
	else {
		nargv[0] = "mread";
		execvp("mread", nargv);
		perror("execvp: mmread") ;
	}
}
