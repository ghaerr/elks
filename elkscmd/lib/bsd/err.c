/*
 * BSD libc compatability functions.
 *
 * Written from scratch by:-
 *
 * Al Riddoch <ajr@ecs.soton.ac.uk>
 *
 * Copyright (C) 1999 Al Riddoch
 *
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "include/bsd_types.h"
#include <stdio.h>

void errx(int eval, const char * fmt, int a, int b, int c, int d)
{
	fprintf(stderr, fmt, a, b, c, d);
	exit(eval);
}
