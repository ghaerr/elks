/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996,1997,1999,2002,2009 Alain Knaff.
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
#include "file.h"

/*
 * Read the clusters given the beginning FAT entry.  Returns 0 on success.
 */

int file_read(FILE *fp, Stream_t *Source, int textmode, int stripmode)
{
	char buffer[16384];
	int pos;
	int ret;

	if (!Source){
		fprintf(stderr,"Couldn't open source file\n");
		return -1;
	}
	
	pos = 0;
	while(1){
		ret = Source->Class->read(Source, buffer, pos, 16384);
		if (ret < 0 ){
			perror("file read");
			return -1;
		}
		if ( ret == 0)
			break;
		if(!fwrite(buffer, 1, ret, fp)){
			perror("write");
			return -1;
		}
		pos += ret;
	}
	return 0;
}
