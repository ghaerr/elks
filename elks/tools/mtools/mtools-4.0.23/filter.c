/*  Copyright 1996,1997,1999,2001-2003,2008,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "codepage.h"

typedef struct Filter_t {
	Class_t *Class;
	int refs;
	Stream_t *Next;
	Stream_t *Buffer;

	int dospos;
	int unixpos;
	int mode;
	int rw;
	int lastchar;
	/* int convertCharset; */
} Filter_t;

#define F_READ 1
#define F_WRITE 2

/* read filter filters out messy dos' bizarre end of lines and final 0x1a's */

static int read_filter(Stream_t *Stream, char *buf, mt_off_t iwhere, size_t len)
{
	DeclareThis(Filter_t);
	int i,j,ret;
	unsigned char newchar;

	off_t where = truncBytes32(iwhere);

	if ( where != This->unixpos ){
		fprintf(stderr,"Bad offset\n");
		exit(1);
	}
	if (This->rw == F_WRITE){
		fprintf(stderr,"Change of transfer direction!\n");
		exit(1);
	}
	This->rw = F_READ;
	
	ret = READS(This->Next, buf, (mt_off_t) This->dospos, len);
	if ( ret < 0 )
		return ret;

	j = 0;
	for (i=0; i< ret; i++){
		if ( buf[i] == '\r' )
			continue;
		if (buf[i] == 0x1a)
			break;
		newchar = buf[i];
		/*
		if (This->convertCharset) newchar = contents_to_unix(newchar);
		*/
		This->lastchar = buf[j++] = newchar;
	}

	This->dospos += i;
	This->unixpos += j;
	return j;
}

static int write_filter(Stream_t *Stream, char *buf, mt_off_t iwhere,
						size_t len)
{
	DeclareThis(Filter_t);
	unsigned int i,j;
	int ret;
	char buffer[1025];
	unsigned char newchar;
	
	off_t where = truncBytes32(iwhere);

	if(This->unixpos == -1)
		return -1;

	if (where != This->unixpos ){
		fprintf(stderr,"Bad offset\n");
		exit(1);
	}
	
	if (This->rw == F_READ){
		fprintf(stderr,"Change of transfer direction!\n");
		exit(1);
	}
	This->rw = F_WRITE;

	j=i=0;
	while(i < 1024 && j < len){
		if (buf[j] == '\n' ){
			buffer[i++] = '\r';
			buffer[i++] = '\n';
			j++;
			continue;
		}
		newchar = buf[j++];
		/*
		if (This->convertCharset) newchar = to_dos(newchar);
		*/
		buffer[i++] = newchar;
	}
	This->unixpos += j;

	ret = force_write(This->Next, buffer, (mt_off_t) This->dospos, i);
	if(ret >0 )
		This->dospos += ret;
	if ( ret != (signed int) i ){
		/* no space on target file ? */
		This->unixpos = -1;
		return -1;
	}
	return j;
}

static int free_filter(Stream_t *Stream)
{
	DeclareThis(Filter_t);
	char buffer=0x1a;

	/* write end of file */
	if (This->rw == F_WRITE)
		return force_write(This->Next, &buffer, (mt_off_t) This->dospos, 1);
	else
		return 0;
}

static Class_t FilterClass = {
	read_filter,
	write_filter,
	0, /* flush */
	free_filter,
	0, /* set geometry */
	get_data_pass_through,
	0,
	0, /* get_dosconvert */
	0  /* discard */
};

Stream_t *open_filter(Stream_t *Next, int convertCharset UNUSEDP)
{
	Filter_t *This;

	This = New(Filter_t);
	if (!This)
		return NULL;
	This->Class = &FilterClass;
	This->dospos = This->unixpos = This->rw = 0;
	This->Next = Next;
	This->refs = 1;
	This->Buffer = 0;
	/*
	  This->convertCharset = convertCharset;
	*/

	return (Stream_t *) This;
}
