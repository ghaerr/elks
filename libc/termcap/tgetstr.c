#include <termcap.h>
#include "t.h"

/* Look up a string-valued capability CAP.
   If AREA is non-null, it points to a pointer to a block in which
   to store the string.  That pointer is advanced over the space used.
   If AREA is null, space is allocated with `malloc'.  */

char *
tgetstr(const char * cap, char **area)
{
  register const char *ptr = termcap_find_capability (termcap_term_entry, cap);
  if (!ptr || (ptr[-1] != '=' && ptr[-1] != '~'))
    return NULL;
  return termcap_tgetst1(ptr, area);
}
