#include <unistd.h>

int brk (const void *addr)
{
   /* NB compact/large model must pass addr residing within app data segment */
   return _brk ((unsigned)addr);
}
