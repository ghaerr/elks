#include <termcap.h>
#include "t.h"

int
tgetflag(const char * cap)
{
  register const char *ptr = termcap_find_capability(termcap_term_entry, cap);
  return ptr && ptr[-1] == ':';
}
