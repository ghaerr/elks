/* refont.c */

/* Author:
 *	Steve Kirkendall
 *	14407 SW Teal Blvd. #C
 *	Beaverton, OR 97005
 *	kirkenda@cs.pdx.edu
 */


/* This file contains the complete source code for the refont program */

/* The refont program converts font designations to the format of your choice.
 * Known font formats are:
 *	-b	overtype notation, using backspaces
 *	-c	overtype notation, using carriage returns
 *	-d	the "dot" command notation used by nroff (doesn't work well)
 *	-e	Epson-compatible line printer codes
 *	-f	the "\f" notation
 *	-x	none (strip out the font codes)
 *
 * Other flags are:
 *	-I	recognize \f and dot notations in input
 *	-F	output a formfeed character between files
 */

#include <stdio.h>
#include "config.h"

/* the program name, for diagnostics */
char	*progname;

/* remembers which output format to use */
int outfmt = 'f';

/* do we allow "dot" and "backslash-f" on input? */
int infmt = 0;

/* do we insert formfeeds between input files? */
int add_form_feed = 0;

main(argc, argv)
	int	argc;	/* number of command-line args */
	char	**argv;	/* values of the command line args */
{
	FILE	*fp;
	int	i, j;
	int	retcode;

	progname = argv[0];

	/* parse the flags */
	i = 1;
	if (i < argc && argv[i][0] == '-' && argv[i][1])
	{
		for (j = 1; argv[i][j]; j++)
		{
			switch (argv[i][j])
			{
			  case 'b':
#if !OSK
			  case 'c':
#endif
			  case 'd':
			  case 'e':
			  case 'f':
			  case 'x':
				outfmt = argv[i][j];
				break;

			  case 'I':
				infmt = 'I';
				break;

			  case 'F':
				add_form_feed = 1;
				break;

			  default:
				usage();
			}
		}
		i++;
	}

	retcode = 0;
	if (i == argc)
	{
		/* probably shouldn't read from keyboard */
		if (isatty(fileno(stdin)))
		{
			usage();
		}

		/* no files named, so use stdin */
		refont(stdin);
	}
	else
	{
		for (; i < argc; i++)
		{
			fp = fopen(argv[i], "r");
			if (!fp)
			{
				perror(argv[i]);
				retcode = 1;
			}
			else
			{
				refont(fp);
				if (i < argc - 1 && add_form_feed)
				{
					putchar('\f');
				}
				fclose(fp);
			}
		}
	}

	exit(retcode);
}

usage()
{
	fputs("usage: ", stderr);
	fputs(progname, stderr);
	fputs(" [-bcdefxFI] [filename]...\n", stderr);
	exit(2);
}

/* This function does the refont thing to a single file */
/* I apologize for the length of this function.  It is gross. */
refont(fp)
	FILE	*fp;
{
	char	textbuf[1025];	/* chars of a line of text */
	char	fontbuf[1025];	/* fonts of chars in fontbuf */
	int	col;		/* where we are in the line */
	int	font;		/* remembered font */
	int	more;		/* more characters to be output? */
	int	ch;

	/* reset some stuff */
	for (col = sizeof fontbuf; --col >= 0; )
	{
		fontbuf[col] = 'R';
		textbuf[col] = '\0';
	}
	col = 0;
	font = 'R';

	/* get the first char - quit if eof */
	while ((ch = getc(fp)) != EOF)
	{
		/* if "dot" command, read the rest of the command */
		if (infmt == 'I' && ch == '.' && col == 0)
		{

			textbuf[col++] = '.';
			textbuf[col++] = getc(fp);
			textbuf[col++] = getc(fp);
			textbuf[col++] = ch = getc(fp);

			/* is it a font line? */
			font = 0;
			if (textbuf[1] == 'u' && textbuf[2] == 'l')
			{
				font = 'U';
			}
			else if (textbuf[1] == 'b' && textbuf[2] == 'o')
			{
				font = 'B';
			}
			else if (textbuf[1] == 'i' && textbuf[2] == 't')
			{
				font = 'I';
			}

			/* if it is, discard the stuff so far but remember font */
			if (font)
			{
				while (col > 0)
				{
					textbuf[--col] = '\0';
				}
			}
			else /* some other format line - write it literally */
			{
				while (ch != '\n')
				{
					textbuf[col++] = ch = getc(fp);
				}
				fputs(textbuf, fp);
				while (col > 0)
				{
					textbuf[--col] = '\0';
				}
			}
			continue;
		}

		/* is it a "\f" formatted font? */
		if (infmt == 'I' && ch == '\\')
		{
			ch = getc(fp);
			if (ch == 'f')
			{
				font = getc(fp);
			}
			else
			{
				textbuf[col++] = '\\';
				textbuf[col++] = ch;
			}
			continue;
		}

		/* is it an Epson font? */
		if (ch == '\033')
		{
			ch = getc(fp);
			switch (ch)
			{
			  case '4':
				font = 'I';
				break;

			  case 'E':
			  case 'G':
				font = 'B';
				break;

			  case '5':
			  case 'F':
			  case 'H':
				font = 'R';
				break;

			  case '-':
				font = (getc(fp) & 1) ? 'U' : 'R';
				break;
			}
			continue;
		}

		/* control characters? */
		if (ch == '\b')
		{
			if (col > 0)
				col--;
			continue;
		}
		else if (ch == '\t')
		{
			do
			{
				if (textbuf[col] == '\0')
				{
					textbuf[col] = ' ';
				}
				col++;
			} while (col & 7);
			continue;
		}
#if !OSK
		else if (ch == '\r')
		{
			col = 0;
			continue;
		}
#endif
		else if (ch == ' ')
		{
			if (textbuf[col] == '\0')
			{
				textbuf[col] = ' ';
				fontbuf[col] = font;
				col++;
			}
			continue;
		}

		/* newline? */
		if (ch == '\n')
		{
			more = 0;
			for (col = 0, font = 'R'; textbuf[col]; col++)
			{
				if (fontbuf[col] != font)
				{
					switch (outfmt)
					{
					  case 'd':
						putchar('\n');
						switch (fontbuf[col])
						{
						  case 'B':
							fputs(".bo\n", stdout);
							break;

						  case 'I':
							fputs(".it\n", stdout);
							break;

						  case 'U':
							fputs(".ul\n", stdout);
							break;
						}
						while (textbuf[col] == ' ')
						{
							col++;
						}
						break;

					  case 'e':
						switch (fontbuf[col])
						{
						  case 'B':
							fputs("\033E", stdout);
							break;

						  case 'I':
							fputs("\0334", stdout);
							break;

						  case 'U':
							fputs("\033-1", stdout);
							break;

						  default:
							switch (font)
							{
							  case 'B':
								fputs("\033F", stdout);
								break;

							  case 'I':
							  	fputs("\0335", stdout);
							  	break;

							  case 'U':
							  	fputs("\033-0", stdout);
							  	break;
							}
						}
						break;

					  case 'f':
					  	putchar('\\');
					  	putchar('f');
					  	putchar(fontbuf[col]);
					  	break;
					}

					font = fontbuf[col];
				}

				if (fontbuf[col] != 'R' && textbuf[col] != ' ')
				{
					switch (outfmt)
					{
					  case 'b':
						if (fontbuf[col] == 'B')
						{
							putchar(textbuf[col]);
						}
						else
						{
							putchar('_');
						}
						putchar('\b');
						break;
#if !OSK
					  case 'c':
					  	more = col + 1;
					  	break;
#endif
					}
				}

				putchar(textbuf[col]);
			}

#if !OSK
			/* another pass? */
			if (more > 0)
			{
				putchar('\r');
				for (col = 0; col < more; col++)
				{
					switch (fontbuf[col])
					{
					  case 'I':
					  case 'U':
						putchar('_');
						break;

					  case 'B':
						putchar(textbuf[col]);
						break;

					  default:
						putchar(' ');
					}
				}
			}
#endif /* not OSK */

			/* newline */
			if (font != 'R')
			{
				switch (outfmt)
				{
				  case 'f':
					putchar('\\');
					putchar('f');
					putchar('R');
					break;

				  case 'e':
					switch (font)
					{
					  case 'B':
						fputs("\033F", stdout);
						break;

					  case 'I':
					  	fputs("\0335", stdout);
					  	break;

					  case 'U':
					  	fputs("\033-0", stdout);
					  	break;
					}
				}
			}
			putchar('\n');

			/* reset some stuff */
			for (col = sizeof fontbuf; --col >= 0; )
			{
				fontbuf[col] = 'R';
				textbuf[col] = '\0';
			}
			col = 0;
			font = 'R';
			continue;
		}

		/* normal character */
		if (font != 'R')
		{
			textbuf[col] = ch;
			fontbuf[col] = font;
		}
		else if (textbuf[col] == '_')
		{
			textbuf[col] = ch;
			fontbuf[col] = 'U';
		}
		else if (textbuf[col] && textbuf[col] != ' ' && ch == '_')
		{
			fontbuf[col] = 'U';
		}
		else if (textbuf[col] == ch)
		{
			fontbuf[col] = 'B';
		}
		else
		{
			textbuf[col] = ch;
		}
		col++;
	}
}
