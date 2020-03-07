#include <stdlib.h>
#include <unistd.h>
#include "t.h"

void
termcap_memory_out(void)
{
  write (2, "virtual memory exhausted\n", 25);
  exit (1);
}
