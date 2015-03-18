/*
 * Copyright (C) 1989 by Kenneth Almquist.  All rights reserved.
 * This file is part of ash, which is distributed under the terms specified
 * by the Ash General Public License.  See the file named LICENSE.
 */

#include <stdio.h>

char *commandname;


void error(char *msg, ...)
{
      fprintf(stderr, "%s: %s\n", commandname, msg);
      exit(2);
}
