#include <unistd.h>

void *sbrk(int increment)
{
   void *new_brk;

   if (_sbrk (increment, &new_brk))
	   return (void *) (intptr_t) -1;

   return new_brk;
}
