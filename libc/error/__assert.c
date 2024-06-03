/* Copyright (C) 1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void errput(const char *str)
{
   write(2, str, strlen(str));
}

void
__assert(const char *assertion, const char *filename, int linenumber)
{
   errput("Failed assertion '");
   errput(assertion);
   errput("' in file ");
   errput(filename);
   errput(" at line ");
   errput(uitoa(linenumber));
   errput(".\n");
   abort();
}
