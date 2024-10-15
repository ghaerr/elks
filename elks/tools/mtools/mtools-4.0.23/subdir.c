/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996,1997,2001,2002,2009 Alain Knaff.
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
#include "vfat.h"
#include "file.h"
#include "buffer.h"

/*
 * Find the directory and load a new dir_chain[].  A null directory
 * is OK.  Returns a 1 on error.
 */


void bufferize(Stream_t **Dir)
{
	Stream_t *BDir;

	if(!*Dir)
		return;
	BDir = buf_init(*Dir, 64*16384, 512, MDIR_SIZE);
	if(!BDir){
		FREE(Dir);
		*Dir = NULL;
	} else
		*Dir = BDir;
}
