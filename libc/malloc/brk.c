#include <unistd.h>

int _brk (void *);

int brk (void * addr)
{
   return _brk (addr);
}
