/*  Copyright 1996-1999,2001,2002,2009 Alain Knaff.
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
#include "llong.h"

/*
 * Copy the data from source to target
 */

int copyfile(Stream_t *Source, Stream_t *Target)
{
	char buffer[8*16384];
	mt_off_t pos;
	int ret;
	ssize_t retw;
/*	size_t len;*/
	mt_size_t mt_len;

	if (!Source){
		fprintf(stderr,"Couldn't open source file\n");
		return -1;
	}

	if (!Target){
		fprintf(stderr,"Couldn't open target file\n");
		return -1;
	}

	pos = 0;
	GET_DATA(Source, 0, &mt_len, 0, 0);
	while(1){
		ret = READS(Source, buffer, (mt_off_t) pos, 8*16384);
		if (ret < 0 ){
			perror("file read");
			return -1;
		}
		if(!ret)
			break;
		if(got_signal)
			return -1;
		if (ret == 0)
			break;
		if ((retw = force_write(Target, buffer,
					(mt_off_t) pos, (size_t) ret)) != ret){
			if(retw < 0 )
				perror("write in copy");
			else
				fprintf(stderr,
					"Short write %lu instead of %d\n",
					(unsigned long) retw, ret);
			if(errno == ENOSPC)
				got_signal = 1;
			return ret;
		}
		pos += ret;
	}
	return 0;
}
