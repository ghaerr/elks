#  Copyright 1997 Marco Nelissen
#  This file is part of mtools.
#
#  Mtools is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Mtools is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
#
#       Makefile for Mtools
#
# check the Configure file for some examples of device-specific setups
# Berkeley flavors of Unix should include -DBSD in the CFLAGS.  Pick
# a lock method... either -DLOCKF, -DFLOCK, or -DFCNTL and put that
# string in the CFLAGS line below.

#
# rudimentary makefile for building mtools 3.1 on the BeOS
#

CC=mwcc -O7

OBJS=buffer.o codepage.o codepages.o config.o copyfile.o devices.o \
directory.o expand.o fat.o fat_free.o file.o file_name.o file_read.o \
filter.o force_io.o hash.o init.o match.o mainloop.o mattrib.o mbadblocks.o \
mcd.o mcopy.o mdel.o mdir.o mformat.o minfo.o misc.o missFuncs.o \
mk_direntry.o mlabel.o mmd.o mmount.o mmove.o mpartition.o mzip.o mtools.o \
parse.o plain_io.o precmd.o privileges.o scsi.o signal.o stream.o \
streamcache.o subdir.o toupper.o tty.o vfat.o xdf_io.o

all: mtools mkmanifest

$(OBJS): config.h

config.h: config.h.Be
	cp config.h.Be config.h

mtools:	$(OBJS)
	$(CC) -o mtools $(OBJS)

mkmanifest: mkmanifest.o
	$(CC) -o mkmanifest mkmanifest.c
