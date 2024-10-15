#ifdef OS_linux

/*  Copyright 1996-2002,2009 Alain Knaff.
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
#include <sys/types.h>

#ifdef HAVE_SYS_SYSMACROS_H

#include <sys/sysmacros.h>
#ifndef MAJOR
#define MAJOR(dev) major(dev)
#endif  /* MAJOR not defined */
#ifndef MINOR
#define MINOR(dev) minor(dev)
#endif  /* MINOR not defined */

#else
 
#include <linux/fs.h>        /* get MAJOR/MINOR from Linux kernel */
#ifndef major
#define major(x) MAJOR(x)
#endif

#endif /* HAVE_SYS_SYSMACROS_H */

#include <linux/fd.h>
#include <linux/fdreg.h>
#include <linux/major.h>


typedef struct floppy_raw_cmd RawRequest_t;

UNUSED(static __inline__ void RR_INIT(struct floppy_raw_cmd *request))
{
	request->data = 0;
	request->length = 0;
	request->cmd_count = 9;
	request->flags = FD_RAW_INTR | FD_RAW_NEED_SEEK | FD_RAW_NEED_DISK
#ifdef FD_RAW_SOFTFAILUE
		| FD_RAW_SOFTFAILURE | FD_RAW_STOP_IF_FAILURE
#endif
		;
	request->cmd[1] = 0;
	request->cmd[6] = 0;
	request->cmd[7] = 0x1b;
	request->cmd[8] = 0xff;
	request->reply_count = 0;
}

UNUSED(static __inline__ void RR_SETRATE(struct floppy_raw_cmd *request, uint8_t rate))
{
	request->rate = rate;
}

UNUSED(static __inline__ void RR_SETDRIVE(struct floppy_raw_cmd *request,int drive))
{
	request->cmd[1] = (request->cmd[1] & ~3) | (drive & 3);
}

UNUSED(static __inline__ void RR_SETTRACK(struct floppy_raw_cmd *request,uint8_t track))
{
	request->cmd[2] = track;
}

UNUSED(static __inline__ void RR_SETPTRACK(struct floppy_raw_cmd *request,
				       int track))
{
	request->track = track;
}

UNUSED(static __inline__ void RR_SETHEAD(struct floppy_raw_cmd *request, uint8_t head))
{
	if(head)
		request->cmd[1] |= 4;
	else
		request->cmd[1] &= ~4;
	request->cmd[3] = head;
}

UNUSED(static __inline__ void RR_SETSECTOR(struct floppy_raw_cmd *request, 
					   uint8_t sector))
{
	request->cmd[4] = sector;
	request->cmd[6] = sector-1;
}

UNUSED(static __inline__ void RR_SETSIZECODE(struct floppy_raw_cmd *request, 
					     uint8_t sizecode))
{
	request->cmd[5] = sizecode;
	request->cmd[6]++;
	request->length += 128 << sizecode;
}

#if 0
static inline void RR_SETEND(struct floppy_raw_cmd *request, int end)
{
	request->cmd[6] = end;
}
#endif

UNUSED(static __inline__ void RR_SETDIRECTION(struct floppy_raw_cmd *request, 
					      int direction))
{
	if(direction == MT_READ) {
		request->flags |= FD_RAW_READ;
		request->cmd[0] = FD_READ & ~0x80;
	} else {
		request->flags |= FD_RAW_WRITE;
		request->cmd[0] = FD_WRITE & ~0x80;
	}
}


UNUSED(static __inline__ void RR_SETDATA(struct floppy_raw_cmd *request, 
					 caddr_t data))
{
	request->data = data;
}


#if 0
static inline void RR_SETLENGTH(struct floppy_raw_cmd *request, int length)
{
	request->length += length;
}
#endif

UNUSED(static __inline__ void RR_SETCONT(struct floppy_raw_cmd *request))
{
#ifdef FD_RAW_MORE
	request->flags |= FD_RAW_MORE;
#endif
}


UNUSED(static __inline__ int RR_SIZECODE(struct floppy_raw_cmd *request))
{
	return request->cmd[5];
}



UNUSED(static __inline__ int RR_TRACK(struct floppy_raw_cmd *request))
{
	return request->cmd[2];
}


UNUSED(static __inline__ int GET_DRIVE(int fd))
{
	struct MT_STAT statbuf;

	if (MT_FSTAT(fd, &statbuf) < 0 ){
		perror("stat");
		return -1;
	}
	  
	if (!S_ISBLK(statbuf.st_mode) ||
	    MAJOR(statbuf.st_rdev) != FLOPPY_MAJOR)
		return -1;
	
	return MINOR( statbuf.st_rdev );
}



/* void print_message(RawRequest_t *raw_cmd,char *message);*/
int send_one_cmd(int fd, RawRequest_t *raw_cmd, const char *message);
int analyze_one_reply(RawRequest_t *raw_cmd, int *bytes, int do_print);


#endif
