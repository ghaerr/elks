/* ctags.c */

/* Author:
 *	Steve Kirkendall
 *	14407 SW Teal Blvd. #C
 *	Beaverton, OR 97005
 *	kirkenda@cs.pdx.edu
 */


/* This file contains the complete source to the ctags program. */

/* Special abilities:
 * Can also make a "refs" file for use by the "ref" program.
 */

/* Limitations:
 * This version of ctags always writes its output to the file "tags".
 * It assumes that every command-line argument (but "-r") is a C source file.
 * It does not sort the list of tags, unless CFLAGS=-DSORT.
 * It does not recognize duplicate definitions.
 * It does not try to handle "static" functions in a clever way.
 * It probably won't scan ANSI-C source code very well.
 */

/* Implementation:
 * Lines are scanned one-at-a-time.
 * The context of lines is tracked via a finite state machine.
 * Contexts are:
 *	EXPECTFN - we're looking for a function name.
 *	ARGS	 - between function name and its opening {
 *	BODY	 - we found a function name, skip to end of body.
 *
 * Function tags are referenced by a search string, so that lines may be
 * inserted or deleted without mucking up the tag search.
 *
 * Macro tags are referenced by their line number, because 1) they usually
 * occur near the top of a file, so their line# won't change much; 2)  They
 * often contain characters that are hard to search for; and 3)  Their #define
 * line is likely to be altered.
 *
 * Each line of the resulting "tags" file describes one tag.  Lines begin with
 * the tag name, then a tab, then the file name, then a tab, and then either
 * a line number or a slash-delimited search string.
 */

#include <ctype.h>
#include <stdio.h>
#include "config.h"

#define REFS	"refs"

#if OSK
#define NUMFMT	"%%.%ds\t%%s\t%%ld\n"
#define SRCHFMT	"%%.%ds\t%%s\t/^%%s$/\n"
#define MAINFMT	"M%%.%ds\t%%s\t/^%%s$/\n"
static char fmt[256];
#else
#define NUMFMT	"%.*s\t%s\t%ld\n"
#define SRCHFMT	"%.*s\t%s\t/^%s$/\n"
#define MAINFMT	"M%.*s\t%s\t/^%s$/\n"
#endif

#ifdef VERBOSE
# define SAY(x)	fprintf(stderr, "%s\n", x);
#else
# define SAY(x)
#endif

#define EXPECTFN 1
#define	ARGS	 2
#define BODY	 3

extern char	*fgets();

char		*progname;	/* argv[0], used for diagnostic output */

main(argc, argv)
	int	argc;
	char	**argv;
{
	FILE	*fp;
	int	i;
	FILE	*refs;	/* used to write to the refs file */

#if MSDOS || TOS
	char **wildexpand();
	argv=wildexpand(&argc, argv);
#endif
	/* notice the program name */
	progname = argv[0];

	/* create the "refs" file if first arg is "-r" */
	if (argc > 1 && !strcmp(argv[1], "-r"))
	{
		/* delete the "-r" flag from the args list */
		argc--;
		argv++;

		/* open the "refs" file for writing */
		refs = fopen(REFS, "w");
		if (!refs)
		{
			fprintf(stderr, "%s: could not create \"%s\"\n", progname, REFS);
			exit(2);
		}
	}
	else
	{
		refs = (FILE *)0;
	}

	/* process each file named on the command line, or complain if none */
	if (argc > 1)
	{
		/* redirect stdout to go to the "tags" file */
		if (!freopen("tags", "w", stdout))
		{
			fprintf(stderr, "%s: could not create \"%s\"\n", progname, TAGS);
			exit(2);
		}

		for (i = 1; i < argc; i++)
		{
			/* process this named file */
			fp = fopen(argv[i], "r");
			if (!fp)
			{
				fprintf(stderr, "%s: could not read \"%s\"\n", progname, argv[i]);
				continue;
			}
			ctags(fp, argv[i], refs);
			fclose(fp);
		}
#ifdef SORT
		/* This is a hack which will sort the tags list.   It should
		 * on UNIX and Minix.  You may have trouble with csh.   Note
		 * that the tags list only has to be sorted if you intend to
		 * use it with the real vi;  elvis permits unsorted tags.
		 */
		fflush(stdout);
#if OSK
		fclose(stdout);
		system("qsort tags >-_tags; -nx; del tags; rename _tags tags");
#else	
		system("sort tags >_tags$$; mv _tags$$ tags");
#endif
#endif
		exit(0);
	}
	else
	{
		fprintf(stderr, "usage: %s *.[ch]\n", progname);
		exit(2);
	}
}


/* this function finds all tags in a given file */
ctags(fp, name, refs)
	FILE	*fp;		/* stream of the file to scan */
	char	*name;		/* name of the file being scanned */
	FILE	*refs;		/* NULL, or where to write refs lines */
{
	int	context;	/* context - either EXPECTFN, ARGS, or BODY */
	long	lnum;		/* line number */
	char	text[1000];	/* a line of text from the file */
	char	*scan;		/* used for searching through text */
	int	len;		/* length of the line */

	/* for each line of the file... */
	for (context = EXPECTFN, lnum = 1; fgets(text, sizeof text, fp); lnum++)
	{
#ifdef VERBOSE
		switch(context)
		{
		  case EXPECTFN:	scan = "EXPECTFN";	break;
		  case ARGS:		scan = "ARGS    ";	break;
		  case BODY:		scan = "BODY    ";	break;
		  default:		scan = "context?";
		}
		fprintf(stderr, "%s:%s", scan, text);
#endif

		/* start of body? */
		if (text[0] == '{')
		{
			context = BODY;
			SAY("Start of BODY");
			continue;
		}

		/* argument line, to be written to "refs" ? */
		if (refs && context == ARGS)
		{
			if (text[0] != '\t')
			{
				putc('\t', refs);
			}
			fputs(text, refs);
			SAY("Argument line");
			continue;
		}

		/* ignore empty or indented lines */
		if (text[0] <= ' ')
		{
			SAY("Empty or indented");
			continue;
		}

		/* end of body? */
		if (text[0] == '}')
		{
			context = EXPECTFN;
			SAY("End of BODY");
			continue;
		}

		/* ignore lines in the body of a function */
		if (context != EXPECTFN)
		{
			SAY("BODY or ARGS");
			continue;
		}

		/* strip the newline */
		len = strlen(text);
		text[--len] = '\0';

		/* a preprocessor line? */
		if (text[0] == '#')
		{
			/* find the preprocessor directive */
			for (scan = &text[1]; isspace(*scan); scan++)
			{
			}

			/* if it's a #define, make a tag out of it */
			if (!strncmp(scan, "define", 6))
			{
				/* find the start of the symbol name */
				for (scan += 6; isspace(*scan); scan++)
				{
				}

				/* find the length of the symbol name */
				for (len = 1;
				     isalnum(scan[len]) || scan[len] == '_';
				     len++)
				{
				}
#if OSK
				sprintf(fmt, NUMFMT, len);
				printf(fmt, scan, name, lnum);
#else			
				printf(NUMFMT, len, scan, name, lnum);
#endif
			}
			SAY("Preprocessor line");
			continue;
		}

		/* an extern or static declaration? */
		if (text[len - 1] == ';'
		 || !strncmp(text, "extern", 6)
		 || !strncmp(text, "EXTERN", 6)
		 || !strncmp(text, "static", 6)
		 || !strncmp(text, "PRIVATE", 7))
		{
			SAY("Extern or static");
			continue;
		}

		/* if we get here & the first punctuation other than "*" is
		 * a "(" which is immediately preceded by a name, then
		 * assume the name is that of a function.
		 */
		for (scan = text; *scan; scan++)
		{
			if (ispunct(*scan)
			 && !isspace(*scan) /* in BSD, spaces are punctuation?*/
			 && *scan != '*' && *scan != '_' && *scan != '(')
			{
				SAY("Funny punctuation");
				goto ContinueContinue;
			}

			if (*scan == '(')
			{
				/* permit 0 or 1 spaces between name & '(' */
				if (scan > text && scan[-1] == ' ')
				{
					scan--;
				}

				/* find the start & length of the name */
				for (len = 0, scan--;
				     scan >= text && (isalnum(*scan) || *scan == '_');
				     scan--, len++)
				{
				}
				scan++;

				/* did we find a function? */
				if (len > 0)
				{
					/* found a function! */
					if (len == 4 && !strncmp(scan, "main", 4))
					{
#if OSK
						sprintf(fmt, MAINFMT, strlen(name) - 2);
						printf(fmt, name, name, text);
#else			
						printf(MAINFMT, strlen(name) - 2, name, name, text);
#endif
					}
#if OSK
					sprintf(fmt, SRCHFMT, len);
					printf(fmt, scan, name, text);
#else				
					printf(SRCHFMT, len, scan, name, text);
#endif
					context = ARGS;

					/* add a line to refs, if needed */
					if (refs)
					{
						fputs(text, refs);
						putc('\n', refs);
					}

					goto ContinueContinue;
				}
			}
			else
			{
				SAY("No parenthesis");
			}
		}
		SAY("No punctuation");

ContinueContinue:;
	}
}

#if MSDOS || TOS
#define		WILDCARD_NO_MAIN
#include	"wildcard.c"
#endif
