#ifndef MTOOLS_FILE_H
#define MTOOLS_FILE_H
/*  Copyright 1996,1997,2001,2002,2009,2011 Alain Knaff.
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
#include "mtoolsDirentry.h"

Stream_t *OpenFileByDirentry(direntry_t *entry);
Stream_t *OpenRoot(Stream_t *Dir);
void printFat(Stream_t *Stream);
void printFatWithOffset(Stream_t *Stream, off_t offset);
direntry_t *getDirentry(Stream_t *Stream);
#endif
