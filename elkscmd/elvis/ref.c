/* ref.c */

/* Author:
 *	Steve Kirkendall
 *	14407 SW Teal Blvd. #C
 *	Beaverton, OR 97005
 *	kirkenda@cs.pdx.edu
 */


/* This program looks up the declarations of library functions. */

#include <stdio.h>

/* This is the list of files that are searched. */
#ifdef OSK
char *refslist[] = {
	"refs",
	"/dd/usr/src/lib/refs",
	"../lib/refs",
	"/dd/usr/local/lib/refs",
};
#else
char *refslist[] = {
	"refs",
	"/usr/src/lib/refs",
	"../lib/refs",
	"/usr/local/lib/refs"
};
#endif
#define NREFS	(sizeof refslist / sizeof(char *))

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	i;	/* used to step through the refslist */

	/* make sure our arguments are OK */
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s function_name\n", *argv);
		exit(2);
	}

	/* check for the function in each database */
	for (i = 0; i < NREFS; i++)
	{
		if (lookinfor(refslist[i], argv[1]))
		{
			exit(0);
		}
	}

	fprintf(stderr, "%s: don't know about %s\n", argv[0], argv[1]);
	exit(2);
}


/* This function checks a single file for the function.  Returns 1 if found */
int lookinfor(filename, func)
	char	*filename;	/* name of file to look in */
	char	*func;		/* name of function to look for */
{
	FILE	*fp;	/* stream used to access the database */
	char	linebuf[300];
		/* NOTE: in actual use, the first character of linebuf is */
		/* set to ' ' and then we use all EXCEPT the 1st character */
		/* everywhere in this function.  This is because the func */
		/* which examines the linebuf could, in some circumstances */
		/* examine the character before the used part of linebuf; */
		/* we need to control what happens then.		   */


	/* open the database file */
	fp = fopen(filename, "r");
	if (!fp)
	{
		return 0;
	}

	/* find the desired entry */
	*linebuf = ' ';
	do
	{
		if (!fgets(linebuf + 1, (sizeof linebuf) - 1, fp))
		{
			fclose(fp);
			return 0;
		}
	} while (!desired(linebuf + 1, func));

	/* print it */
	do
	{
		fputs(linebuf + 1, stdout);
	} while (fgets(linebuf + 1, sizeof linebuf, fp) && linebuf[1] == '\t');

	/* cleanup & exit */
	fclose(fp);
	return 1;
}


/* This function checks to see if a given line is the first line of the */
/* entry the user wants to see.  If it is, return non-0 else return 0   */
desired(line, word)
	char	*line;	/* the line buffer */
	char	*word;	/* the string it should contain */
{
	static	wlen = -1;/* length of the "word" variable's value */
	register char *scan;

	/* if this line starts with a tab, it couldn't be desired */
	if (*line == '\t')
	{
		return 0;
	}

	/* if we haven't found word's length yet, do so */
	if (wlen < 0)
	{
		wlen = strlen(word);
	}

	/* search for the word in the line */
	for (scan = line; *scan != '('; scan++)
	{
	}
	while (*--scan == ' ')
	{
	}
	scan -= wlen;
	if (scan < line - 1 || *scan != ' ' && *scan != '\t' && *scan != '*')
	{
		return 0;
	}
	scan++;
	return !strncmp(scan, word, wlen);
}
