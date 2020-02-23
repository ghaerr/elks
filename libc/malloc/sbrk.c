#include <unistd.h>

int _sbrk (intptr_t, void **);

void * sbrk (intptr_t increment)
{
   void * new_brk;
   if (_sbrk (increment, &new_brk))
	   return (void *) -1;

   return new_brk;
}
