#include <sys/types.h>
#include "t.h"

/* Search entry BP for capability CAP.
   Return a pointer to the capability (in BP) if found,
   0 if not found.  */

const char *
termcap_find_capability (register const char *bp, register const char *cap)
{
  for (; *bp; bp++)
    if (bp[0] == ':'
	&& bp[1] == cap[0]
	&& bp[2] == cap[1])
      return &bp[4];
  return NULL;
}
