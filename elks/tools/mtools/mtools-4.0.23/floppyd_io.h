#ifndef MTOOLS_FLOPPYDIO_H
#define MTOOLS_FLOPPYDIO_H

/*  Copyright 1999 Peter Schlaile.
 *  Copyright 1998,2000-2002,2009 Alain Knaff.
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

#ifdef USE_FLOPPYD

#include "stream.h"

typedef uint8_t Byte;
typedef uint32_t Dword;
typedef uint64_t Qword;

#define DWORD_ERR ((Dword) -1)

/*extern int ConnectToFloppyd(const char* name, Class_t** ioclass);*/
Stream_t *FloppydOpen(struct device *dev, 
		      char *name, int mode, char *errmsg,
		      mt_size_t *maxSize);

#define FLOPPYD_DEFAULT_PORT 5703

#define FLOPPYD_PROTOCOL_VERSION_OLD 10
#define FLOPPYD_PROTOCOL_VERSION 11

#define FLOPPYD_CAP_EXPLICIT_OPEN 1 /* explicit open. Useful for 
				     * clean signalling of readonly disks */
#define FLOPPYD_CAP_LARGE_SEEK 2    /* large seeks */

enum FloppydOpcodes {
	OP_READ,
	OP_WRITE,
	OP_SEEK,
	OP_FLUSH,
	OP_CLOSE,
	OP_IOCTL,
	OP_OPRO,
	OP_OPRW,
	OP_SEEK64
};

enum AuthErrorsEnum {
	AUTH_SUCCESS,
	AUTH_PACKETOVERSIZE,
	AUTH_AUTHFAILED,
	AUTH_WRONGVERSION,
	AUTH_DEVLOCKED,
	AUTH_BADPACKET,
	AUTH_IO_ERROR
};


UNUSED(static inline void cork(int sockhandle, int on))
{
#ifdef TCP_CORK
	if(setsockopt(sockhandle, IPPROTO_TCP, 
		      TCP_CORK, (char *)&on, sizeof(on)) < 0) {
		perror("setsockopt cork");
	}
#else
	on = 1 ^ on;
	if(setsockopt(sockhandle, IPPROTO_TCP, 
		      TCP_NODELAY, (char *)&on, sizeof(on)) < 0) {
		perror("setsockopt nodelay");
	}
#endif
}


#endif
#endif
