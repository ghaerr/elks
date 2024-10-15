#ifndef MTOOLS_STREAM_H
#define MTOOLS_STREAM_H

/*  Copyright 1996-1999,2001,2002,2005,2006,2008,2009,2011 Alain Knaff.
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

typedef struct Stream_t {
	struct Class_t *Class;
	int refs;
	struct Stream_t *Next;
	struct Stream_t *Buffer;
} Stream_t;

#include "mtools.h"
#include "msdos.h"

#include "llong.h"

doscp_t *get_dosConvert_pass_through(Stream_t *Stream);

typedef struct Class_t {
	int (*read)(Stream_t *, char *, mt_off_t, size_t);
	int (*write)(Stream_t *, char *, mt_off_t, size_t);
	int (*flush)(Stream_t *);
	int (*freeFunc)(Stream_t *);
	int (*set_geom)(Stream_t *, device_t *, device_t *, int media,
					union bootsector *);
	int (*get_data)(Stream_t *, time_t *, mt_size_t *, int *, int *);
	int (*pre_allocate)(Stream_t *, mt_size_t);

	doscp_t *(*get_dosConvert)(Stream_t *);

	int (*discard)(Stream_t *);
} Class_t;

#define READS(stream, buf, address, size) \
((stream)->Class->read)( (stream), (char *) (buf), (address), (size) )

#define WRITES(stream, buf, address, size) \
((stream)->Class->write)( (stream), (char *) (buf), (address), (size) )

#define SET_GEOM(stream, dev, orig_dev, media, boot) \
(stream)->Class->set_geom( (stream), (dev), (orig_dev), (media), (boot) )

#define GET_DATA(stream, date, size, type, address) \
(stream)->Class->get_data( (stream), (date), (size), (type), (address) )

#define PRE_ALLOCATE(stream, size) \
(stream)->Class->pre_allocate((stream), (size))

#define GET_DOSCONVERT(stream)			\
	(stream)->Class->get_dosConvert((stream))

#define DISCARD(stream)			\
	(stream)->Class->discard((stream))

int flush_stream(Stream_t *Stream);
Stream_t *copy_stream(Stream_t *Stream);
int free_stream(Stream_t **Stream);

#define FLUSH(stream) \
flush_stream( (stream) )

#define FREE(stream) \
free_stream( (stream) )

#define COPY(stream) \
copy_stream( (stream) )


#define DeclareThis(x) x *This = (x *) Stream

int force_write(Stream_t *Stream, char *buf, mt_off_t start, size_t len);
int force_read(Stream_t *Stream, char *buf, mt_off_t start, size_t len);

int get_data_pass_through(Stream_t *Stream, time_t *date, mt_size_t *size,
						  int *type, int *address);

int read_pass_through(Stream_t *Stream, char *buf, mt_off_t start, size_t len);
int write_pass_through(Stream_t *Stream, char *buf, mt_off_t start, size_t len);

mt_off_t sectorsToBytes(Stream_t *This, off_t off);

mt_size_t getfree(Stream_t *Stream);
int getfreeMinBytes(Stream_t *Stream, mt_size_t ref);

Stream_t *find_device(char drive, int mode, struct device *out_dev,
		      union bootsector *boot,
		      char *name, int *media, mt_size_t *maxSize,
		      int *isRop);

#endif

