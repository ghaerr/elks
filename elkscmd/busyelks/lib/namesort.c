/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Utility routines.
 */

#include "../sash.h"

/*
 * Sort routine for list of filenames.
 */
int
namesort(p1, p2)
	char	**p1;
	char	**p2;
{
	return strcmp(*p2, *p1);
}

