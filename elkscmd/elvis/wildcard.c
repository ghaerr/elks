/* wildcard.c */

/* Author:
 *	Guntram Blohm
 *	Buchenstrasse 19
 *	7904 Erbach, West Germany
 *	Tel. ++49-7305-6997
 *	sorry - no regular network connection
 */

/* this program implements wildcard expansion for elvis/dos. It works
 * like UNIX echo, but uses the dos wildcard conventions
 * (*.* matches all files, * matches files without extension only,
 * filespecs may contain drive letters, wildcards not allowed in directory
 * names).
 *
 * It is also #included into ctags.c, ref.c, ...; in this case,
 * we don't want a main function here.
 */

#include <stdio.h>
#include <ctype.h>
#ifdef	__TURBOC__
#include <dir.h>
#endif
#ifdef	M_I86
#define	findfirst(a,b,c)	_dos_findfirst(a,c,b)
#define	findnext		_dos_findnext
#define	ffblk			find_t
#define	ff_name			name
#include <dos.h>
#endif
#ifdef	M68000
#include <stat.h>
#include <osbind.h>
#define	findfirst(a,b,c)	(Fsetdta(b), (Fsfirst(a,c)))
#define	findnext(x)		(Fsnext())
#define	ff_name	d_fname
#endif
#define	MAXFILES	1000

int pstrcmp();
extern char *calloc();

char *files[MAXFILES];
int nfiles;

#ifndef	WILDCARD_NO_MAIN

main(argc, argv)
	char **argv;
{
	int i;

	for (i=1; i<argc; i++)
		expand(argv[i]);
	if (nfiles)
		printf("%s", files[0]);
	for (i=1; i<nfiles; i++)
	{
		printf(" %s", files[i]);
	}
	putchar('\n');
	return 0;
}

#else
char **wildexpand(argc, argv)
	int *argc;
	char **argv;
{
	int i;
	
	for (i=0; i<*argc; i++)
		expand(argv[i]);
	*argc=nfiles;
	return files;
}	
#endif

expand(name)
	char *name;
{
	char *filespec;
	int wildcard=0;
#ifdef	M68000
	DMABUFFER findbuf;
#else
	struct ffblk findbuf;
#endif
	int err;
	char buf[80];
	int lastn;

	strcpy(buf, name);
	for (filespec=buf; *filespec; filespec++)
		;

	while (--filespec>=buf)
	{	if (*filespec=='?' || *filespec=='*')
			wildcard=1;
		if (*filespec=='/' || *filespec=='\\' || *filespec==':')
			break;
	}
	if (!wildcard)
		addfile(buf);
	else
	{
		lastn=nfiles;
		filespec++;
		if ((err=findfirst(buf, &findbuf, 0))!=0)
			addfile(buf);
		while (!err)
		{
			strcpy(filespec, findbuf.ff_name);
			addfile(buf);
			err=findnext(&findbuf);
		}
		if (lastn!=nfiles)
			qsort(files+lastn, nfiles-lastn, sizeof(char *), pstrcmp);
	}
}

addfile(buf)
	char *buf;
{
	char *p;

	for (p=buf; *p; p++)
		*p=tolower(*p);

	if (nfiles<MAXFILES && (files[nfiles]=calloc(strlen(buf)+1, 1))!=0)
		strcpy(files[nfiles++], buf);
}

int pstrcmp(a, b)
	char **a, **b;
{
	return strcmp(*a, *b);
}
