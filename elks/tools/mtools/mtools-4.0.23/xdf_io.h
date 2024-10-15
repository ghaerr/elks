#ifndef MTOOLS_XDFIO_H
#define MTOOLS_XDFIO_H

/*  Copyright 1996,1997,2001,2002,2009 Alain Knaff.
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

#include "msdos.h"
#include "stream.h"

struct xdf_info {
  unsigned int FatSize;
  unsigned int RootDirSize;
  unsigned int BadSectors;
};

Stream_t *XdfOpen(struct device *dev, char *name,
		  int mode, char *errmsg, struct xdf_info *info);

#endif
