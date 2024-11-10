#ifndef MTOOLS_DIRENTRY_H
#define MTOOLS_DIRENTRY_H
/*  Copyright 1998,2000-2002,2005,2008-2010 Alain Knaff.
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
#include "vfat.h"

typedef struct direntry_t {
	struct Stream_t *Dir;
	/* struct direntry_t *parent; parent level */	
	int entry; /* slot in parent directory (-3 if root) */
	struct directory dir; /* descriptor in parent directory (random if 
			       * root)*/
	wchar_t name[MAX_VNAMELEN+1]; /* name in its parent directory, or 
				       * NULL if root */
	int beginSlot; /* begin and end slot, for delete */
	int endSlot;
} direntry_t;

#include "stream.h"

int vfat_lookup(direntry_t *entry, const char *filename, int length, int flags,
		char *shortname, size_t shortname_len,
		char *longname, size_t longname_len);

struct directory *dir_read(direntry_t *entry, int *error);

void initializeDirentry(direntry_t *entry, struct Stream_t *Dir);
int isNotFound(direntry_t *entry);
direntry_t *getParent(direntry_t *entry);
void dir_write(direntry_t *entry);
void low_level_dir_write(direntry_t *entry);
void low_level_dir_write_end(Stream_t *Dir, int entry);
int fatFreeWithDirentry(direntry_t *entry);
int labelit(struct dos_name_t *dosname,
	    char *longname,
	    void *arg0,
	    direntry_t *entry);
int isSubdirOf(Stream_t *inside, Stream_t *outside);
char *getPwd(direntry_t *entry);
void fprintPwd(FILE *f, direntry_t *entry, int escape);
void fprintShortPwd(FILE *f, direntry_t *entry);
int write_vfat(Stream_t *, dos_name_t *, char *, int, direntry_t *);

void wipeEntry(struct direntry_t *entry);

void dosnameToDirentry(const struct dos_name_t *n, struct directory *dir);

int lookupForInsert(Stream_t *Dir,
		    direntry_t *direntry,
		    struct dos_name_t *dosname,
		    char *longname,
		    struct scan_state *ssp, 
		    int ignore_entry,
		    int source_entry,
		    int pessimisticShortRename,
		    int use_longname);
#endif
