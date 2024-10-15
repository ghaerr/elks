/*  Copyright 1997,2000-2003,2007-2010 Alain Knaff.  This file is
 *  part of mtools.
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
#include "file.h"
#include "mtoolsDirentry.h"
#include "file_name.h"

void initializeDirentry(direntry_t *entry, Stream_t *Dir)
{
	memset(entry, 0, sizeof(direntry_t));
	entry->entry = -1;
	entry->Dir = Dir;
	entry->beginSlot = 0;
	entry->endSlot = 0;
}

int isNotFound(direntry_t *entry)
{
	return entry->entry == -2;
}

direntry_t *getParent(direntry_t *entry)
{
	return getDirentry(entry->Dir);
}


static size_t getPathLen(direntry_t *entry)
{
	size_t length=0;

	while(1) {
		if(entry->entry == -3) /* rootDir */
			return length + 3;
		
		length += 1 + wcslen(entry->name);
		entry = getDirentry(entry->Dir);
	}
}

static char *sprintPwd(direntry_t *entry, char *ptr, size_t *len_available)
{
	if(entry->entry == -3) {
		*ptr++ = getDrive(entry->Dir);
		*ptr++ = ':';
		*ptr++ = '/';
		(*len_available) -= 3;
	} else {
		size_t bytes_converted;
		ptr = sprintPwd(getDirentry(entry->Dir), ptr, len_available);
		if(ptr[-1] != '/') {
			*ptr++ = '/';
			(*len_available)--;
		}
		bytes_converted = wchar_to_native(entry->name, ptr,
						  MAX_VNAMELEN, *len_available);
		ptr += bytes_converted;
		(*len_available) -= bytes_converted;
	}
	return ptr;
}


#ifdef HAVE_WCHAR_H
#define NEED_ESCAPE L"\"$\\"
#else
#define NEED_ESCAPE "\"$\\"
#endif

static void _fprintPwd(FILE *f, direntry_t *entry, int recurs, int escape)
{
	if(entry->entry == -3) {
		putc(getDrive(entry->Dir), f);
		putc(':', f);
		if(!recurs)
			putc('/', f);
	} else {
		_fprintPwd(f, getDirentry(entry->Dir), 1, escape);
		if (escape && wcspbrk(entry->name, NEED_ESCAPE)) {
			wchar_t *ptr;
			putc('/', f);
			for(ptr = entry->name; *ptr; ptr++) {
				if (wcschr(NEED_ESCAPE, *ptr))
					putc('\\', f);
				putwc(*ptr, f);
			}
		} else {
			char tmp[4*MAX_VNAMELEN+1];
			WCHAR_TO_NATIVE(entry->name,tmp,MAX_VNAMELEN);
			fprintf(f, "/%s", tmp);
		}
	}
}

void fprintPwd(FILE *f, direntry_t *entry, int escape)
{
	if (escape)
		putc('"', f);
	_fprintPwd(f, entry, 0, escape);
	if(escape)
		putc('"', f);
}

static void _fprintShortPwd(FILE *f, direntry_t *entry, int recurs)
{
	if(entry->entry == -3) {
		putc(getDrive(entry->Dir), f);
		putc(':', f);
		if(!recurs)
			putc('/', f);
	} else {
		int i,j;
		_fprintShortPwd(f, getDirentry(entry->Dir), 1);
		putc('/',f);
		for(i=7; i>=0 && entry->dir.name[i] == ' ';i--);
		for(j=0; j<=i; j++)
			putc(entry->dir.name[j],f);
		for(i=2; i>=0 && entry->dir.ext[i] == ' ';i--);
		if(i > 0)
			putc('.',f);
		for(j=0; j<=i; j++)
			putc(entry->dir.ext[j],f);
	}
}

void fprintShortPwd(FILE *f, direntry_t *entry)
{
	_fprintShortPwd(f, entry, 0);
}

char *getPwd(direntry_t *entry)
{
	size_t size;
	char *ret;
	char *end;
	size_t buf_size;

	size = getPathLen(entry);
	buf_size = size*4+1;
	ret = malloc(buf_size);
	if(!ret)
		return 0;
	end = sprintPwd(entry, ret, &buf_size);
	*end = '\0';
	return ret;
}

int isSubdirOf(Stream_t *inside, Stream_t *outside)
{
	while(1) {
		if(inside == outside) /* both are the same */
			return 1;
		if(getDirentry(inside)->entry == -3) /* root directory */
			return 0;
		/* look further up */
		inside = getDirentry(inside)->Dir;
	}			
}
