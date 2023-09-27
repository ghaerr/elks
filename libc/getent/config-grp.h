/*
 * config-grp.h - This file is part of the libc-8086/grp package for ELKS,
 * Copyright (C) 1995, 1996 Nat Friedman <ndf@linux.mit.edu>.
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _CONFIG_GRP_H
#define _CONFIG_GRP_H

/*
 * GR_MAX_LINE_LEN is the maximum number of characters per line in the group file.
 * GR_MAX_MEMBERS is the maximum number of members of any given group.
 */

#define GR_MAX_LINE_LEN 128

/* GR_MAX_MEMBERS = (GR_MAX_LINE_LEN-(24+3+6))/9 */
#define GR_MAX_MEMBERS  11

#endif /* _CONFIG_GRP_H */
