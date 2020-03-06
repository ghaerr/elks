#include <stdlib.h>
#include "t.h"

char *
termcap_xrealloc(char * ptr, unsigned size)
{
  register char *tem = realloc(ptr, size);

  if (tem == NULL)
    termcap_memory_out();

  return tem;
}
