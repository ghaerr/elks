#ifdef L_alloca
#include <malloc.h>

#include "_malloc.h"

/* This alloca is based on the same concept as the EMACS fallback alloca.
 * It should probably be considered Copyright the FSF under the GPL.
 */
static mem __wcnear *alloca_stack = 0;

void *
alloca(size_t size)
{
   auto char probe;		/* Probes stack depth: */
   register mem __wcnear *hp;

   /*
    * Reclaim garbage, defined as all alloca'd storage that was allocated
    * from deeper in the stack than currently.
    */

   for (hp = alloca_stack; hp != 0;)
      if (m_deep(hp) < &probe)
      {
	 register mem __wcnear *np = m_next(hp);
	 free((void *) hp);	/* Collect garbage.  */
	 hp = np;		/* -> next header.  */
      }
      else
	 break;			/* Rest are not deeper.  */

   alloca_stack = hp;		/* -> last valid storage.  */
   if (size == 0)
      return 0;			/* No allocation required.  */

   hp = (mem __wcnear *) (*__alloca_alloc) (sizeof(mem)*2 + size);
   if (hp == 0)
      return hp;

   m_next(hp) = alloca_stack;
   m_deep(hp) = &probe;
   alloca_stack = hp;

   /* User storage begins just after header.  */
   return (void *) (hp + 2);
}
#endif				/* L_alloca */
