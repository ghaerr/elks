/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

/*
 * This is a combined alloca/malloc package. It uses a classic algorithm
 * and so may be seen to be quite slow compared to more modern routines
 * with 'nasty' distributions.
 */

#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysctl.h>

#include "_malloc.h"

#define MAX_INT ((int)(((unsigned)-1)>>1))

#if VERBOSE == 1
int __debug_level = 1;
#endif

/*
 * The chunk_list pointer is either NULL or points to a chunk in a
 * circular list of all the free blocks in memory
 */

static mem __wcnear *chunk_list = 0;

/*
 * This function takes a pointer to a block of memory and inserts it into
 * the chain of memory chunks
 */
static void
__insert_chunk(mem __wcnear *mem_chunk)
{
   register mem __wcnear *p1, __wcnear *p2;
   if (chunk_list == 0)		/* Simple case first */
   {
      chunk_list = mem_chunk;
      m_next(mem_chunk) = (union mem_cell __wcnear *)mem_chunk;
      debug("FIRST CHUNK", mem_chunk);
      return;
   }
   p1 = mem_chunk;
   p2 = chunk_list;

   do
   {
      if (p1 > p2)
      {
	 if (m_next(p2) <= p2)
	 {			/* We're at the top of the chain, p1 is
				 * higher */

	    if (p2 + m_size(p2) == p1)
	    {			/* Good, stick 'em together */
	       debug("INSERT CHUNK", mem_chunk);
	       m_size(p2) += m_size(p1);
	       debug("JOIN 1", p2);
	    }
	    else
	    {
	       m_next(p1) = m_next(p2);
	       m_next(p2) = (union mem_cell __wcnear *)p1;
	       debug("INSERT CHUNK", mem_chunk);
	       debug("FROM", p2);
	    }
	    return;
	 }
	 if (m_next(p2) > p1)
	 {
	    /* In chain, p1 between p2 and next */

	    m_next(p1) = m_next(p2);
	    m_next(p2) = (union mem_cell __wcnear *)p1;
	    debug("INSERT CHUNK", mem_chunk);
	    debug("FROM", p2);

	    /* Try to join above */
	    if (p1 + m_size(p1) == m_next(p1))
	    {
	       m_size(p1) += m_size(m_next(p1));
	       m_next(p1) = m_next(m_next(p1));
	       debug("JOIN 2", p1);
	    }
	    /* Try to join below */
	    if (p2 + m_size(p2) == p1)
	    {
	       m_size(p2) += m_size(p1);
	       m_next(p2) = m_next(p1);
	       debug("JOIN 3", p2);
	    }
	    chunk_list = p2;	/* Make sure it's valid */
	    return;
	 }
      }
      else if (p1 < p2)
      {
	 if (m_next(p2) <= p2 && p1 < m_next(p2))
	 {
	    /* At top of chain, next is bottom of chain, p1 is below next */

	    m_next(p1) = m_next(p2);
	    m_next(p2) = (union mem_cell __wcnear *)p1;
	    debug("INSERT CHUNK", mem_chunk);
	    debug("FROM", p2);
	    chunk_list = p2;

	    if (p1 + m_size(p1) == m_next(p1))
	    {
	       if (p2 == m_next(p1))
		  chunk_list = p1;
	       m_size(p1) += m_size(m_next(p1));
	       m_next(p1) = m_next(m_next(p1));
	       debug("JOIN 4", p1);
	    }
	    return;
	 }
      }
      chunk_list = p2;		/* Save for search */
      p2 = m_next(p2);
   }
   while (p2 != chunk_list);

   /* If we get here we have a problem, ignore it, maybe it'll go away */
   debug("DROPPED CHUNK", mem_chunk);
}

/*
 * This function will search for a chunk in memory of at least 'mem_size'
 * when found, if the chunk is too big it'll be split, and pointer to the
 * chunk returned. If none is found NULL is returned.
 */
static mem __wcnear *
__search_chunk(unsigned int mem_size)
{
   register mem __wcnear *p1, __wcnear *p2;
   if (chunk_list == 0)		/* Simple case first */
      return 0;

   /* Search for a block >= the size we want */
   p1 = m_next(chunk_list);
   p2 = chunk_list;
   do
   {
      debug("CHECKED", p1);
      if (m_size(p1) >= mem_size)
	 break;

      p2 = p1;
      p1 = m_next(p1);
   }
   while (p2 != chunk_list);

   /* None found, exit */
   if (m_size(p1) < mem_size)
      return 0;

   /* If it's exactly right remove it */
   if (m_size(p1) < mem_size + 2)
   {
      debug("FOUND RIGHT", p1);
      chunk_list = m_next(p2) = m_next(p1);
      if (chunk_list == p1)
	 chunk_list = 0;
      return p1;
   }

   debug("SPLIT", p1);
   /* Otherwise split it */
   m_next(p2) = (union mem_cell __wcnear *)(p1 + mem_size);
   chunk_list = p2;

   p2 = m_next(p2);
   m_size(p2) = m_size(p1) - mem_size;
   m_next(p2) = m_next(p1);
   m_size(p1) = mem_size;
   if (chunk_list == p1)
      chunk_list = p2;

   p1[1].size = (unsigned int)0xAAAA;   /* canary, not required */
   debug("INSERT CHUNK", p2);
   debug("FOUND CHUNK", p1);
   debug("LIST IS", chunk_list);
   return p1;
}

size_t malloc_usable_size(void *ptr)
{
    if (ptr == 0)
        return 0;
    return (m_size(((mem *) ptr) - 1) - 1) * sizeof(mem);
}

void *
malloc(size_t size)
{
   register mem __wcnear *ptr = 0;
   register unsigned int sz;

#if VERBOSE == 1
   if (chunk_list == 0)
        sysctl(CTL_GET, "malloc.debug", &__debug_level);
#endif

   errno = 0;
   if (size == 0)
      return 0;			/* ANSI STD, no error */

   /* Minor oops here, sbrk has a signed argument */
   if((int)size < 0 || size > (((unsigned)-1) >> 1) - sizeof(mem) * 3)
   {
      errno = ENOMEM;
      return 0;
   }

   sz = size + sizeof(mem) * 2 - 1;
   sz /= sizeof(mem);

#ifdef MINALLOC
   if (sz < MINALLOC)
      sz = MINALLOC;
#endif

   dprintf("MALLOC %u\n", sz * sizeof(mem));
#if VERBOSE > 1
   {
      static mem arr[2];
      m_size(arr) = sz;
      debug("WANTED", arr);
   }
#endif

#ifdef __MINI_MALLOC__
   __alloca_alloc = malloc;	/* We'll be messing with the heap now TVM */
#endif

#ifdef LAZY_FREE
   ptr = __search_chunk(sz);
   if (ptr == 0)
   {
#endif

      /* First deal with the freed list */
      if (__freed_list)
      {
	 while (__freed_list)
	 {
	    ptr = __freed_list;
	    __freed_list = m_next(__freed_list);

	    if (m_size(ptr) == sz)	/* Oh! Well that's lucky ain't it :-) */
	    {
	       debug("RETURN (exact from freelist)", ptr);
	       return ptr + 1;
	    }

	    __insert_chunk(ptr);
	 }
	 ptr = m_next(chunk_list);
         if (ptr + m_size(ptr) == (void *) sbrk(0))
	 {
	    /* Time to free for real */
	    m_next(chunk_list) = m_next(ptr);
	    if (ptr == m_next(ptr))
	       chunk_list = 0;
	    free(ptr + 1);
	 }
#ifdef LAZY_FREE
	 ptr = __search_chunk(sz);
#endif
      }
#ifndef LAZY_FREE
      ptr = __search_chunk(sz);
#endif
      if (ptr == 0)
      {
#ifdef MCHUNK
         unsigned int alloc;
         if (sz < MCHUNK)
                alloc = sizeof(mem) * (MCHUNK * ((sz + MCHUNK - 1) / MCHUNK) - 1);
         else alloc = sz * sizeof(mem);
	 ptr = __mini_malloc(alloc);
	 if (ptr)
	    __insert_chunk(ptr - 1);
#if MCHUNK >= 256
	 else		/* Oooo, near end of RAM */
	 {
	    unsigned int needed = alloc;
	    for(alloc/=2; alloc>256 && needed; )
	    {
	       ptr = __mini_malloc(alloc);
	       if (ptr)
	       {
	          if( alloc > needed ) needed = 0; else needed -= alloc;
	          __insert_chunk(ptr - 1);
	       }
	       else     alloc/=2;
	    }
	 }
#endif
	 ptr = __search_chunk(sz);
	 if (ptr == 0)
#endif
	 {
#ifndef MCHUNK
	    ptr = __mini_malloc(size);
#endif
            if (ptr == 0)
            {
                errno = ENOMEM;
                dprintf("FAIL\n");
                debug("FAIL", 0);
                return 0;       /* don't far-extend near ptr in large model */
            }
            debug("RETURN (new chunk)", ptr - 1);
            return ptr;
	 }
      }
#ifdef LAZY_FREE
   }
#endif

   ptr[1].size = (unsigned int)0x5555;  /* canary, not required */
   debug("RETURN", ptr);
   return ptr + 1;
}
