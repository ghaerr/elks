/* atari.c */

/* Author:
 *	Guntram Blohm
 *	Buchenstrasse 19
 *	7904 Erbach, West Germany
 *	Tel. ++49-7305-6997
 *	sorry - no regular network connection
 */

/*
 * This file contains the 'standard' functions which are not supported
 * by Atari/Mark Williams, and some other TOS-only requirements.
 */
 
#include "config.h"
#include "vi.h"

#if TOS
#include <osbind.h>

/* vi uses mode==0 only ... */
int access(file, mode)
	char *file;
{
	int fd=Fopen(file, 0);
	if (fd<0)
		return -1;
	Fclose(fd);
	return 0;
}

char *mktemp(template)
	char *template;
{
	return template;
}

/* read -- text mode, compress \r\n to \n
 * warning: might fail when maxlen==1 and at eol
 */

int tread(fd, buf, maxlen)
	int fd;
	char *buf;
	int maxlen;
{
	int i, j, nread=read(fd, buf, (unsigned)maxlen);

	if (nread && buf[nread-1]=='\r')
	{	nread--;
		lseek(fd, -1l, 1);
	}
	for (i=j=0; j<nread; i++,j++)
	{	if (buf[j]=='\r' && buf[j+1]=='\n')
			j++;
		buf[i]=buf[j];
	}
	return i;
}

int twrite(fd, buf, maxlen)
	int fd;
	char *buf;
	int maxlen;
{
	int i, j, nwritten=0, hadnl=0;
	char writbuf[BLKSIZE];

	for (i=j=0; j<maxlen; )
	{
		if ((writbuf[i++]=buf[j++])=='\n')
		{	writbuf[i-1]='\r';
			if (i<BLKSIZE)
				writbuf[i++]='\n';
			else
				hadnl=1;
		}
		if (i==BLKSIZE)
		{
			write(fd, writbuf, (unsigned)i);
			i=0;
		}
		if (hadnl)
		{
			writbuf[i++]='\n';
			hadnl=0;
		}
	}
	if (i)
		write(fd, writbuf, (unsigned)i);
	return j;
}
#endif
