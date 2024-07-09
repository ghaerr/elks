#include <malloc.h>
#include <unistd.h>

#include "_malloc.h"

void
free(void * ptr)
{
	register mem __wcnear *top;
	register mem __wcnear *chk = (mem __wcnear *) ptr;

	if (chk == 0)
		return;		/* free(NULL) - be nice */
	chk--;

#ifdef __MINI_MALLOC__
 try_this:;
#endif
	top = (mem __wcnear *) sbrk(0);
	if (chk + m_size(chk) == top) {
		__noise("FREE brk", chk);
		brk(top - m_size(chk));
		/*
		 * Adding this code allow free to release blocks in any order; they
		 * can still only be allocated from the top of the heap tho.
		 */
#ifdef __MINI_MALLOC__
		if (__alloca_alloc == __mini_malloc && __freed_list) {
			chk = __freed_list;
			__freed_list = m_next(__freed_list);
			goto try_this;
		}
#endif
	} else {		/* Nope, not sure where this goes, leave
				 * it for malloc to deal with */
#ifdef __MINI_MALLOC__
		if (__freed_list || chk > __freed_list) {
			m_next(chk) = __freed_list;
			__freed_list = chk;
		} else {
			register mem __wcnear *prev;
			prev = __freed_list;
			for (top = __freed_list; top && top > chk;
			     prev = top, top = m_next(top)) ;
			m_next(chk) = top;
			m_next(prev) = chk;
		}
#else
		m_next(chk) = (union mem_cell __wcnear *)__freed_list;
		__freed_list = chk;
#endif
		__noise("ADD LIST", chk);
	}
}
