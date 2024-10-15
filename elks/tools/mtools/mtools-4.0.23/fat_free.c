/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-1998,2001,2002,2009 Alain Knaff.
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
#include "fsP.h"
#include "mtoolsDirentry.h"

/*
 * Remove a string of FAT entries (delete the file).  The argument is
 * the beginning of the string.  Does not consider the file length, so
 * if FAT is corrupted, watch out!
 */

int fat_free(Stream_t *Dir, unsigned int fat)
{
	Stream_t *Stream = GetFs(Dir);
	DeclareThis(Fs_t);
	unsigned int next_no_step;
					/* a zero length file? */
	if (fat == 0)
		return(0);
	/* CONSTCOND */
	while (!This->fat_error) {
		/* get next cluster number */
		next_no_step = fatDecode(This,fat);
		/* mark current cluster as empty */
		fatDeallocate(This,fat);
		if (next_no_step >= This->last_fat)
			break;
		fat = next_no_step;
	}
	return(0);
}

int fatFreeWithDir(Stream_t *Dir, struct directory *dir)
{
	unsigned int first;

	if((!strncmp(dir->name,".      ",8) ||
	    !strncmp(dir->name,"..     ",8)) &&
	   !strncmp(dir->ext,"   ",3)) {
		fprintf(stderr,"Trying to remove . or .. entry\n");
		return -1;
	}

	first = START(dir);
  	if(fat32RootCluster(Dir))
		first |= STARTHI(dir) << 16;
	return fat_free(Dir, first);
}

int fatFreeWithDirentry(direntry_t *entry)
{
	return fatFreeWithDir(entry->Dir, &entry->dir);
}
    
