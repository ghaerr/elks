/* virec.c */

/* Author:
 *	Steve Kirkendall
 *	14407 SW Teal Blvd. #C
 *	Beaverton, OR 97005
 *	kirkenda@cs.pdx.edu
 */

/* This file contains the file recovery program */


#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "vi.h"
#if TOS
# include <stat.h>
#else
# if OSK
#  include "osk.h"
# else
#  include <sys/stat.h>
# endif
#endif

extern char	*getenv();
struct stat	stbuf;
BLK		hdr;
BLK		text;

/* the name of the directory where tmp files are stored. */
char o_directory[30] = TMPDIR;

char	*progname;

main(argc, argv)
	int	argc;
	char	**argv;
{
	char	*tmp;
	void recover();
#if MSDOS || TOS
	char **wildexpand();
	argv = wildexpand(&argc, argv);
#endif
	progname = argv[0];
	/* set the o_directory variable */
	if ((tmp = getenv("TMP"))	/* yes, ASSIGNMENT! */
	 || (tmp = getenv("TEMP")))	/* yes, ASSIGNMENT! */
	{
		strcpy(o_directory, tmp);
	}
	if (argc >= 3 && !strcmp(argv[1], "-d"))
	{
		strcpy(o_directory, argv[2]);
		argc -= 2;
		argv += 2;
	}
	/* process the arguments */
	if (argc < 2)
	{
		/* maybe stdin comes from a file? */
		if (isatty(0))
		{
			fprintf(stderr, "usage: %s [-d tmpdir] lostfile...\n", progname);
		}
		else if (read(0, &hdr, (unsigned)BLKSIZE) != BLKSIZE)
		{
			fprintf(stderr, "couldn't get header\n");
		}
		else
		{
			copytext(0, stdout);
		}
	}
	else
	{
		while (--argc > 0)
		{
			recover(*++argv);
		}
	}
	exit(0);
}


/* This function recovers a single file */
void recover(filename)
	char	*filename;
{
	char		tmpname[100];
	int		tmpfd;
	FILE		*fp;
	long		mtime;
	int		i, j;
	int		sum;	/* used for calculating a checksum for this */
	char		*scan;

	/* get the file's status info */
	if (stat(filename, &stbuf) < 0)
	{
		/* if serious error, give up on this file */
		if (errno != ENOENT)
		{
			perror(filename);
			return;
		}

		/* else fake it for a new file */
		stat(".", &stbuf);
#if OSK
		stbuf.st_mode = S_IREAD;
#else
		stbuf.st_mode = S_IFREG;
#endif
		stbuf.st_mtime = 0L;
	}

	/* generate a checksum from the file's name */
	for (sum = 0, scan = filename + strlen(filename);
	     --scan >= filename && (isascii(*scan) && isalnum(*scan) || *scan == '.');
	     sum = sum + *scan)
	{
	}
	sum &= 0xf;

	/* find the tmp file */
#if MSDOS || TOS
	/* MS-Dos doesn't allow multiple slashes, but supports drives
	 * with current directories.
	 * This relies on TMPNAME beginning with "%s\\"!!!!
	 */
	strcpy(tmpname, o_directory);
	if ((i = strlen(tmpname)) && !strchr(":/\\", tmpname[i-1]))
		tmpname[i++]=SLASH;
	sprintf(tmpname+i, TMPNAME+3, sum, stbuf.st_ino, stbuf.st_dev);
#else
	sprintf(tmpname, TMPNAME, o_directory, sum, stbuf.st_ino, stbuf.st_dev);
#endif
	tmpfd = open(tmpname, O_RDONLY | O_BINARY);
	if (tmpfd < 0)
	{
		perror(tmpname);
		return;
	}

	/* make sure the file hasn't been modified more recently */
	mtime = stbuf.st_mtime;
	fstat(tmpfd, &stbuf);
	if (stbuf.st_mtime < mtime)
	{
		printf("\"%s\" has been modified more recently than its recoverable version\n", filename);
		puts("Do you still want to recover it?\n");
		puts("\ty - Yes, discard the current version and recover it.\n");
		puts("\tn - No, discard the recoverable version and keep the current version\n");
		puts("\tq - Quit without doing anything for this file.\n");
		puts("Enter y, n, or q --> ");
		fflush(stdout);
		for (;;)
		{
			switch (getchar())
			{
			  case 'y':
			  case 'Y':
				goto BreakBreak;

			  case 'n':
			  case 'N':
				close(tmpfd);
				unlink(tmpname);
				return;

			  case 'q':
			  case 'Q':
				close(tmpfd);
				return;
			}
		}
BreakBreak:;
	}

	/* make sure this tmp file is intact */
	if (read(tmpfd, &hdr, (unsigned)BLKSIZE) != BLKSIZE)
	{
		fprintf(stderr, "%s: bad header in tmp file\n", filename);
		close(tmpfd);
		unlink(tmpname);
		return;
	}
	for (i = j = 1; i < MAXBLKS && hdr.n[i]; i++)
	{
		if (hdr.n[i] > j)
		{
			j = hdr.n[i];
		}
	}
	lseek(tmpfd, (long)j * (long)BLKSIZE, 0);
	if (read(tmpfd, &text, (unsigned)BLKSIZE) != BLKSIZE)
	{
		fprintf(stderr, "%s: bad data block in tmp file\n", filename);
		close(tmpfd);
		unlink(tmpname);
		return;
	}

	/* open the normal text file for writing */
	fp = fopen(filename, "w");
	if (!fp)
	{
		perror(filename);
		close(tmpfd);
		return;
	}

	/* copy the text */
	copytext(tmpfd, fp);

	/* cleanup */
	close(tmpfd);
	fclose(fp);
	unlink(tmpname);
}


/* This function moves text from the tmp file to the normal file */
copytext(tmpfd, fp)
	int	tmpfd;	/* fd of the tmp file */
	FILE	*fp;	/* the stream to write it to */
{
	int	i;

	/* write the data blocks to the normal text file */
	for (i = 1; i < MAXBLKS && hdr.n[i]; i++)
	{
		lseek(tmpfd, (long)hdr.n[i] * (long)BLKSIZE, 0);
		read(tmpfd, &text, (unsigned)BLKSIZE);
		fputs(text.c, fp);
	}
}

#if MSDOS || TOS
#define		WILDCARD_NO_MAIN
#include	"wildcard.c"
#endif
