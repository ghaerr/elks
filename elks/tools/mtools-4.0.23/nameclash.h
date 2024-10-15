#ifndef MTOOLS_NAMECLASH_H
#define MTOOLS_NAMECLASH_H

/*  Copyright 1996-1998,2000-2002,2008,2009 Alain Knaff.
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

#include "stream.h"

typedef enum clash_action {
	NAMEMATCH_NONE,
	NAMEMATCH_AUTORENAME,
	NAMEMATCH_QUIT,
	NAMEMATCH_SKIP,
	NAMEMATCH_RENAME,
	NAMEMATCH_PRENAME, /* renaming of primary name */
	NAMEMATCH_OVERWRITE,
	NAMEMATCH_ERROR,
	NAMEMATCH_SUCCESS,
	NAMEMATCH_GREW
} clash_action;

/* clash handling structure */
typedef struct ClashHandling_t {
	clash_action action[2];
	clash_action namematch_default[2];
		
	int nowarn;	/* Don't ask, just do default action if name collision*/
	int got_slots;
	int mod_time;
	/* unsigned int dot; */
	char *myname;
	unsigned char *dosname;
	int single;

	int use_longname;
	int ignore_entry;
	int source; /* to prevent the source from overwriting itself */
	int source_entry; /* to account for the space freed up by the original 
					   * name */
	void (*name_converter)(doscp_t *cp,
			       const char *filename, int verbose, 
			       int *mangled, dos_name_t *ans);
	int is_label;
} ClashHandling_t;

/* write callback */
typedef int (write_data_callback)(dos_name_t *,char *, void *, struct direntry_t *);

int mwrite_one(Stream_t *Dir,
	       const char *argname,
	       const char *shortname,
	       write_data_callback *cb,
	       void *arg,
	       ClashHandling_t *ch);

int handle_clash_options(ClashHandling_t *ch, char c);
void init_clash_handling(ClashHandling_t *ch);
Stream_t *createDir(Stream_t *Dir, const char *filename, ClashHandling_t *ch,
		    unsigned char attr, time_t mtime);


#endif
