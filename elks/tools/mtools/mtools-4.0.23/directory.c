/*  Copyright 1995 David C. Niemi
 *  Copyright 1996-2002,2008,2009 Alain Knaff.
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
#include "file.h"
#include "fs.h"
#include "file_name.h"

/* #define DEBUG */

/*
 * Read a directory entry into caller supplied buffer
 */
struct directory *dir_read(direntry_t *entry, int *error)
{
	int n;
	*error = 0;
	if((n=force_read(entry->Dir, (char *) (&entry->dir), 
			 (mt_off_t) entry->entry * MDIR_SIZE, 
			 MDIR_SIZE)) != MDIR_SIZE) {
		if (n < 0) {
			*error = -1;
		}
		return NULL;
	}
	return &entry->dir;
}

/*
 * Make a subdirectory grow in length.  Only subdirectories (not root)
 * may grow.  Returns a 0 on success, 1 on failure (disk full), or -1
 * on error.
 */

int dir_grow(Stream_t *Dir, int size)
{
	Stream_t *Stream = GetFs(Dir);
	DeclareThis(FsPublic_t);
	int ret;
	unsigned int buflen;
	char *buffer;
	
	if (!getfreeMinClusters(Dir, 1))
		return -1;

	buflen = This->cluster_size * This->sector_size;

	if(! (buffer=malloc(buflen)) ){
		perror("dir_grow: malloc");
		return -1;
	}
		
	memset((char *) buffer, '\0', buflen);
	ret = force_write(Dir, buffer, (mt_off_t) size * MDIR_SIZE, buflen);
	free(buffer);
	if(ret < (int) buflen)
		return -1;
	return 0;
}


void low_level_dir_write(direntry_t *entry)
{
	force_write(entry->Dir, 
		    (char *) (&entry->dir), 
		    (mt_off_t) entry->entry * MDIR_SIZE, MDIR_SIZE);
}

void low_level_dir_write_end(Stream_t *Dir, int entry)
{
	char zero = ENDMARK;
	force_write(Dir, &zero, (mt_off_t) entry * MDIR_SIZE, 1);
}

/*
 * Make a directory entry.  Builds a directory entry based on the
 * name, attribute, starting cluster number, and size.  Returns a pointer
 * to a static directory structure.
 */

struct directory *mk_entry(const dos_name_t *dn, unsigned char attr,
			   unsigned int fat, size_t size, time_t date,
			   struct directory *ndir)
{
	struct tm *now;
	time_t date2 = date;
	unsigned char hour, min_hi, min_low, sec;
	unsigned char year, month_hi, month_low, day;

	now = localtime(&date2);
	dosnameToDirentry(dn, ndir);
	ndir->attr = attr;
	ndir->ctime_ms = 0;
	hour = now->tm_hour << 3;
	min_hi = now->tm_min >> 3;
	min_low = now->tm_min << 5;
	sec = now->tm_sec / 2;
	ndir->ctime[1] = ndir->time[1] = hour + min_hi;
	ndir->ctime[0] = ndir->time[0] = min_low + sec;
	year = (now->tm_year - 80) << 1;
	month_hi = (now->tm_mon + 1) >> 3;
	month_low = (now->tm_mon + 1) << 5;
	day = now->tm_mday;
	ndir -> adate[1] = ndir->cdate[1] = ndir->date[1] = year + month_hi;
	ndir -> adate[0] = ndir->cdate[0] = ndir->date[0] = month_low + day;

	set_word(ndir->start, fat & 0xffff);
	set_word(ndir->startHi, fat >> 16);
	set_dword(ndir->size, size);
	return ndir;
}

/*
 * Make a directory entry from base name. This is supposed to be used
 * from places such as mmd for making special entries (".", "..", "/", ...)
 * Thus it doesn't bother with character set conversions
 */
struct directory *mk_entry_from_base(const char *base, unsigned char attr,
				     unsigned int fat, size_t size, time_t date,
				     struct directory *ndir)
{
	struct dos_name_t dn;
	strncpy(dn.base, base, 8);
	strncpy(dn.ext, "   ", 3);
	return mk_entry(&dn, attr, fat, size, date, ndir);
}
