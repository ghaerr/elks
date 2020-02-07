#include <stdio.h>
#include <string.h>

#include "cmd.h"

#define	true_main(argc, argv)		(0)
#define	false_main(argc, argv)	(1)

int
main(int argc, char * argv[])
{
	char * progname = strrchr(argv[0], '/');

	if(!progname || (progname == argv[0] && !progname[1]))
		progname = argv[0];
	else
		progname++;

	/* If our name was called, assume argv[1] is the command */
	if(strcmp(progname, "busyelks") == 0) {
		progname = argv[1];
		argv++;
		argc--;
	}

#if defined(CMD_basename)
	if(!strcmp(progname, "basename"))
		return basename_main(argc, argv);
#endif

#if defined(CMD_dirname)
	if(!strcmp(progname, "dirname"))
		return dirname_main(argc, argv);
#endif

#if defined(CMD_true)
	if(!strcmp(progname, "true"))
		return true_main(argc, argv);
#endif

#if defined(CMD_false)
	if(!strcmp(progname, "false"))
		return false_main(argc, argv);
#endif

	/*
	if(strcmp(progname, "sync") == 0) { sync(); exit(0); }
	*/

	fputs("BusyELKS commands:\n"
#if defined(CMD_basename)
		"\tbasename NAME [SUFFIX]\n"
#endif
#if defined(CMD_dirname)
		"\tdirname NAME\n"
#endif
#if defined(CMD_false)
		"\tfalse\n"
#endif
#if defined(CMD_true)
		"\ttrue\n"
#endif
		, stderr);

	return 1;
}
