/* alias.c */

/* Author:
 *		Peter Reinig
 *		Universitaet Kaiserslautern
 *		Postfach 3049
 *		7650 Kaiserslautern
 *		W. Germany
 *		reinig@physik.uni-kl.de
 */

/* This tiny program executes elvis with the flags that are appropriate
 * for a given command name.  This program is used only on systems that
 * don't allow UNIX-style file links.
 *
 * The benefit of this program is: instead of having 5 copies of elvis
 * on your disk, you only need one copy of elvis and 4 copies of this
 * little program.
 */

#include <stdio.h>
#include "config.h"

#if OSK
#define ELVIS	"/dd/usr/cmds/elvis"
#else
#define ELVIS	"elvis"
#endif

extern char **environ;
extern int errno;
extern char *malloc();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	pid, i, j;
	int	letter;
	char	**argblk;
#if OSK
	extern int chainc();
#endif

	/* allocate enough space for a copy of the argument list, plus a
	 * terminating NULL, plus maybe an added flag.
	 */
	argblk = (char **) malloc((argc + 2) * sizeof(char *));
	if (!argblk)
	{
#if OSK
		_errmsg(errno, "Can't get enough memory\n");
#else
		perror(argv[0]);
#endif
		exit(2);
	}

	/* find the last letter in the invocation name of this program */
	i = strlen(argv[0]);
#if MSDOS || TOS
	/* we almost certainly must bypass ".EXE" or ".TTP" from argv[0] */
	if (i > 4 && argv[0][i - 4] == '.')
		i -= 4;
#endif
	letter = argv[0][i - 1];

	/* copy argv to argblk, possibly inserting a flag such as "-R" */
	argblk[0] = ELVIS;
	i = j = 1;
	switch (letter)
	{
	  case 'w':			/* "view" */
	  case 'W':
		argblk[i++] = "-R";
		break;
#if !OSK
	  case 'x':			/* "ex" */
	  case 'X':
		argblk[i++] = "-e";
		break;
#endif
	  case 't':			/* "input" */
	  case 'T':
		argblk[i++] = "-i";
		break;
	}
	while (j < argc)
	{
		argblk[i++] = argv[j++];
	}
	argblk[i] = (char *)0;

	/* execute the real ELVIS program */
#if OSK
	pid = os9exec(chainc, argblk[0], argblk, environ, 0, 0, 3);
	fprintf(stderr, "%s: cannot execute\n", argblk[0]);
#else
	(void)execvp(argblk[0], argblk);
	perror(ELVIS);
#endif
}
