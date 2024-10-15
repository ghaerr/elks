/*  Copyright 1999-2003,2006,2008,2009 Alain Knaff.
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
#include "stream.h"
#include "fsP.h"
#include "llong.h"
#include "mtools.h"

#if 1
const mt_off_t max_off_t_31 = MAX_OFF_T_B(31); /* Floppyd */
static const mt_off_t max_off_t_32 = MAX_OFF_T_B(32); /* Directory */
const mt_off_t max_off_t_41 = MAX_OFF_T_B(41); /* SCSI */
const mt_off_t max_off_t_seek = MAX_OFF_T_B(SEEK_BITS); /* SCSI */
#else
const mt_off_t max_off_t_31 = MAX_OFF_T_B(10); /* Floppyd */
const mt_off_t max_off_t_41 = MAX_OFF_T_B(10); /* SCSI */
const mt_off_t max_off_t_seek = MAX_OFF_T_B(10); /* SCSI */
#endif

int fileTooBig(mt_off_t off) {
	return (off & ~max_off_t_32) != 0;
}

off_t truncBytes32(mt_off_t off)
{
	if (fileTooBig(off)) {
		fprintf(stderr, "Internal error, offset too big\n");
		exit(1);
	}
	return (off_t) off;
}

mt_off_t sectorsToBytes(Stream_t *Stream, off_t off)
{
	DeclareThis(Fs_t);
	return (mt_off_t) off << This->sectorShift;
}

#if defined HAVE_LLSEEK
# ifndef HAVE_LLSEEK_PROTOTYPE
extern long long llseek (int fd, long long offset, int origin);
# endif
#endif

#if defined HAVE_LSEEK64
# ifndef HAVE_LSEEK64_PROTOTYPE
extern long long lseek64 (int fd, long long offset, int origin);
# endif
#endif


int mt_lseek(int fd, mt_off_t where, int whence)
{
#if defined HAVE_LSEEK64
	if(lseek64(fd, where, whence) >= 0)
		return 0;
	else
		return -1;
#elif defined HAVE_LLSEEK
	if(llseek(fd, where, whence) >= 0)
		return 0;
	else
		return -1;		
#else
	if (lseek(fd, (off_t) where, whence) >= 0)
		return 0;
	else
		return 1;
#endif
}

unsigned int log_2(int size)
{
	unsigned int i;

	for(i=0; i<24; i++) {
		if(1 << i == size)
			return i;
	}
	return 24;
}
