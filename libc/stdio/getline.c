/* Copyright (C) 2004       Manuel Novoa III    <mjn3@codepoet.org>
 *
 * GNU Library General Public License (LGPL) version 2 or later.
 *
 * Dedicated to Toni.  See uClibc/DEDICATION.mjn3 for details.
 */

#include <stdio.h>
#include <unistd.h>

ssize_t getline(char ** restrict lineptr, size_t * restrict n, FILE * restrict stream)
{
	return getdelim(lineptr, n, '\n', stream);
}
