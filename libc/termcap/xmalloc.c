#include <stdlib.h>
#include "t.h"

char *
termcap_xmalloc(unsigned size)
{
  register char *tem = malloc (size);

  if (!tem)
    termcap_memory_out();

  return tem;
}
