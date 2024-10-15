/*  Copyright 1998-2002,2009 Alain Knaff.
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
#include "stream.h"
#include "mtools.h"
#include "fsP.h"
#include "file.h"
#include "htable.h"
#include "mainloop.h"
#include <dirent.h>

typedef struct Dir_t {
	Class_t *Class;
	int refs;
	Stream_t *Next;
	Stream_t *Buffer;

	struct MT_STAT statbuf;
	char *pathname;
	DIR *dir;
#ifdef HAVE_FCHDIR
	int fd;
#endif
} Dir_t;

/*#define FCHDIR_MODE*/

static int get_dir_data(Stream_t *Stream, time_t *date, mt_size_t *size,
			int *type, int *address)
{
	DeclareThis(Dir_t);

	if(date)
		*date = This->statbuf.st_mtime;
	if(size)
		*size = (mt_size_t) This->statbuf.st_size;
	if(type)
		*type = 1;
	if(address)
		*address = 0;
	return 0;
}

static int dir_free(Stream_t *Stream)
{
	DeclareThis(Dir_t);

	Free(This->pathname);
	closedir(This->dir);
	return 0;
}

static Class_t DirClass = { 
	0, /* read */
	0, /* write */
	0, /* flush */
	dir_free, /* free */
	0, /* get_geom */
	get_dir_data ,
	0, /* pre-allocate */
	0, /* get_dosConvert */
	0 /* discard */
};

#ifdef HAVE_FCHDIR
#define FCHDIR_MODE
#endif

int unix_dir_loop(Stream_t *Stream, MainParam_t *mp); 
int unix_loop(Stream_t *Stream, MainParam_t *mp, char *arg, 
	      int follow_dir_link);

int unix_dir_loop(Stream_t *Stream, MainParam_t *mp)
{
	DeclareThis(Dir_t);
	struct dirent *entry;
	char *newName;
	int ret=0;

#ifdef FCHDIR_MODE
	int fd;

	fd = open(".", O_RDONLY);
	if(chdir(This->pathname) < 0) {
		fprintf(stderr, "Could not chdir into %s (%s)\n",
			This->pathname, strerror(errno));
		return -1;
	}
#endif
	while((entry=readdir(This->dir)) != NULL) {
		if(got_signal)
			break;
		if(isSpecial(entry->d_name))
			continue;
#ifndef FCHDIR_MODE
		newName = malloc(strlen(This->pathname) + 1 + 
				 strlen(entry->d_name) + 1);
		if(!newName) {
			ret = ERROR_ONE;
			break;
		}
		strcpy(newName, This->pathname);
		strcat(newName, "/");
		strcat(newName, entry->d_name);
#else
		newName = entry->d_name;
#endif
		ret |= unix_loop(Stream, mp, newName, 0);
#ifndef FCHDIR_MODE
		free(newName);
#endif
	}
#ifdef FCHDIR_MODE
	if(fchdir(fd) < 0)
		perror("Could not chdir back to ..");
	close(fd);
#endif
	return ret;
}

Stream_t *OpenDir(const char *filename)
{
	Dir_t *This;

	This = New(Dir_t);
	
	This->Class = &DirClass;
	This->Next = 0;
	This->refs = 1;
	This->Buffer = 0;
	This->pathname = malloc(strlen(filename)+1);
	if(This->pathname == NULL) {
		Free(This);
		return NULL;
	}
	strcpy(This->pathname, filename);

	if(MT_STAT(filename, &This->statbuf) < 0) {
		Free(This->pathname);
		Free(This);
		return NULL;
	}

	This->dir = opendir(filename);
	if(!This->dir) {
		Free(This->pathname);
		Free(This);
		return NULL;
	}

	return (Stream_t *) This;
}
