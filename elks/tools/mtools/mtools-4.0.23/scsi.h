#ifndef __mtools_scsi_h
#define __mtools_scsi_h
/*  Copyright 1997-1999,2001,2002,2009 Alain Knaff.
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

#define SCSI_READ 0x8
#define SCSI_WRITE 0xA
#define SCSI_IOMEGA 0xC
#define SCSI_INQUIRY 0x12
#define SCSI_MODE_SENSE 0x1a
#define SCSI_START_STOP 0x1b
#define SCSI_ALLOW_MEDIUM_REMOVAL 0x1e
#define SCSI_GROUP1 0x20
#define SCSI_READ_CAPACITY 0x25


typedef enum { SCSI_IO_READ, SCSI_IO_WRITE } scsi_io_mode_t;
int scsi_max_length(void);
int scsi_cmd(int fd, unsigned char cdb[6], int clen, scsi_io_mode_t mode,
	     void *data, size_t len, void *extra_data);
int scsi_open(const char *name, int flags, int mode, void **extra_data);

#endif /* __mtools_scsi_h */
