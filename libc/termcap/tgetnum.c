#include <stdlib.h>
#include <termcap.h>
#include "t.h"

int
tgetnum(const char * cap)
{
  register const char *ptr = termcap_find_capability(termcap_term_entry, cap);
  if (!ptr || ptr[-1] != '#')
    return -1;
  return atoi (ptr);
}
