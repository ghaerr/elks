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
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define MCHUNK		2048	/* Allocation unit in 'mem' elements */
#define XLAZY_FREE		/* If set frees can be infinitly defered */
#define XMINALLOC	32	/* Smallest chunk to alloc in 'mem's */
#define XVERBOSE 		/* Lots of noise, debuging ? */

#undef  malloc
#define MAX_INT ((int)(((unsigned)-1)>>1))

#ifdef VERBOSE
#define noise __noise
#else
#define noise(y,x)
#endif

typedef union mem_cell
{
   union mem_cell *next;	/* A pointer to the next mem */
   unsigned int size;		/* An int >= sizeof pointer */
   char *depth;			/* For the alloca hack */
}
mem;

#define m_size(p)  ((p) [0].size)	/* For malloc */
#define m_next(p)  ((p) [1].next)	/* For malloc and alloca */
#define m_deep(p)  ((p) [0].depth)	/* For alloca */

extern void *__mini_malloc __P ((size_t));
extern void *(*__alloca_alloc) __P ((size_t));

/* Start the alloca with just the dumb version of malloc */

void *(*__alloca_alloc) __P ((size_t)) = __mini_malloc;
mem  *__freed_list = 0;

#ifdef VERBOSE
/* NB: Careful here, stdio may use malloc - so we can't */
static
phex(val)
{
   static char hex[] = "0123456789ABCDEF";
   int   i;
   for (i = sizeof(int)*8-4; i >= 0; i -= 4)
      write(2, hex + ((val >> i) & 0xF), 1);
}

noise(y, x)
char *y;
mem  *x;
{
   write(2, "Malloc ", 7);
   phex(x);
   write(2, " sz ", 4);
   if(x) phex(m_size(x)); else phex(0);
   write(2, " nxt ", 5);
   if(x) phex(m_next(x)); else phex(0);
   write(2, " is ", 4);
   write(2, y, strlen(y));
   write(2, "\n", 1);
}
#endif

//-----------------------------------------------------------------------------

#ifdef L_alloca
/* This alloca is based on the same concept as the EMACS fallback alloca.
 * It should probably be considered Copyright the FSF under the GPL.
 */
static mem *alloca_stack = 0;

void *
alloca(size)
size_t size;
{
   auto char probe;		/* Probes stack depth: */
   register mem *hp;

   /*
    * Reclaim garbage, defined as all alloca'd storage that was allocated
    * from deeper in the stack than currently.
    */

   for (hp = alloca_stack; hp != 0;)
      if (m_deep(hp) < &probe)
      {
	 register mem *np = m_next(hp);
	 free((void *) hp);	/* Collect garbage.  */
	 hp = np;		/* -> next header.  */
      }
      else
	 break;			/* Rest are not deeper.  */

   alloca_stack = hp;		/* -> last valid storage.  */
   if (size == 0)
      return 0;			/* No allocation required.  */

   hp = (mem *) (*__alloca_alloc) (sizeof(mem)*2 + size);
   if (hp == 0)
      return hp;

   m_next(hp) = alloca_stack;
   m_deep(hp) = &probe;
   alloca_stack = hp;

   /* User storage begins just after header.  */
   return (void *) (hp + 2);
}
#endif				/* L_alloca */

//-----------------------------------------------------------------------------

void
free(ptr)
void *ptr;
{
   register mem *top;
   register mem *chk = (mem *) ptr;

   if (chk == 0)
      return;			/* free(NULL) - be nice */
   chk--;

 try_this:;
   top = (mem *) sbrk(0);
   if (chk + m_size(chk) == top)
   {
      noise("FREE brk", chk);
      brk(top-m_size(chk));
      /*
       * Adding this code allow free to release blocks in any order; they
       * can still only be allocated from the top of the heap tho.
       */
#ifdef __MINI_MALLOC__
      if (__alloca_alloc == __mini_malloc && __freed_list)
      {
	 chk = __freed_list;
	 __freed_list = m_next(__freed_list);
	 goto try_this;
      }
#endif
   }
   else
   {				/* Nope, not sure where this goes, leave
				 * it for malloc to deal with */
#ifdef __MINI_MALLOC__
      if( __freed_list || chk > __freed_list )
      { m_next(chk) = __freed_list; __freed_list = chk; }
      else
      {
         register mem *prev;
         prev=__freed_list;
	 for(top=__freed_list; top && top > chk; prev=top, top=m_next(top))
	     ;
         m_next(chk) = top;
	 m_next(prev) = chk;
      }
#else
      m_next(chk) = __freed_list;
      __freed_list = chk;
#endif
      noise("ADD LIST", chk);
   }
}

void *
__mini_malloc(size)
size_t size;
{
   register mem *ptr;
   register unsigned int sz;

   /* First time round this _might_ be odd, But we won't do that! */
   sz = (unsigned int) sbrk(0);
   if (sz & (sizeof(mem) - 1))
      sbrk(4 - (sz & (sizeof(mem) - 1)));

   if (size <= 0)
      return 0;
   /* Minor oops here, sbrk has a signed argument */
   if( size > (((unsigned)-1)>>1)-sizeof(mem)*3 )
   {
      errno = ENOMEM;
      return 0;
   }

   size += sizeof(mem) * 2 - 1;	/* Round up and leave space for size field */
   size /= sizeof(mem);

   ptr = (mem *) sbrk(size * sizeof(mem));
   if ((int) ptr == -1)
      return 0;

   m_size(ptr) = size;
   noise("CREATE", ptr);
   return ptr + 1;
}

//-----------------------------------------------------------------------------

/*
 * The chunk_list pointer is either NULL or points to a chunk in a
 * circular list of all the free blocks in memory
 */

#define Static static

static mem *chunk_list = 0;
Static void __insert_chunk();
Static mem *__search_chunk();

void *
malloc(size)
size_t size;
{
   register mem *ptr = 0;
   register unsigned int sz;

   if (size == 0)
      return 0;			/* ANSI STD */

   sz = size + sizeof(mem) * 2 - 1;
   sz /= sizeof(mem);

#ifdef MINALLOC
   if (sz < MINALLOC)
      sz = MINALLOC;
#endif

#ifdef VERBOSE
   {
      static mem arr[2];
      m_size(arr) = sz;
      noise("WANTED", arr);
   }
#endif

   __alloca_alloc = malloc;	/* We'll be messing with the heap now TVM */

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

	    if (m_size(ptr) == sz)	/* Oh! Well that's lucky ain't it
					 * :-) */
	    {
	       noise("LUCKY MALLOC", ptr);
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
         alloc = sizeof(mem) * (MCHUNK * ((sz + MCHUNK - 1) / MCHUNK) - 1);
	 ptr = __mini_malloc(alloc);
	 if (ptr)
	    __insert_chunk(ptr - 1);
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
	 ptr = __search_chunk(sz);
	 if (ptr == 0)
#endif
	 {
#ifndef MCHUNK
	    ptr = __mini_malloc(size);
#endif
#ifdef VERBOSE
	    if( ptr == 0 )
	       noise("MALLOC FAIL", 0);
	    else
	       noise("MALLOC NOW", ptr - 1);
#endif
	    return ptr;
	 }
      }
#ifdef LAZY_FREE
   }
#endif

#ifdef VERBOSE
   ptr[1].size = 0x55555555;
#endif
   noise("MALLOC RET", ptr);
   return ptr + 1;
}

/*
 * This function takes a pointer to a block of memory and inserts it into
 * the chain of memory chunks
 */

Static void
__insert_chunk(mem_chunk)
mem  *mem_chunk;
{
   register mem *p1, *p2;
   if (chunk_list == 0)		/* Simple case first */
   {
      m_next(mem_chunk) = chunk_list = mem_chunk;
      noise("FIRST CHUNK", mem_chunk);
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
	       noise("INSERT CHUNK", mem_chunk);
	       m_size(p2) += m_size(p1);
	       noise("JOIN 1", p2);
	    }
	    else
	    {
	       m_next(p1) = m_next(p2);
	       m_next(p2) = p1;
	       noise("INSERT CHUNK", mem_chunk);
	       noise("FROM", p2);
	    }
	    return;
	 }
	 if (m_next(p2) > p1)
	 {
	    /* In chain, p1 between p2 and next */

	    m_next(p1) = m_next(p2);
	    m_next(p2) = p1;
	    noise("INSERT CHUNK", mem_chunk);
	    noise("FROM", p2);

	    /* Try to join above */
	    if (p1 + m_size(p1) == m_next(p1))
	    {
	       m_size(p1) += m_size(m_next(p1));
	       m_next(p1) = m_next(m_next(p1));
	       noise("JOIN 2", p1);
	    }
	    /* Try to join below */
	    if (p2 + m_size(p2) == p1)
	    {
	       m_size(p2) += m_size(p1);
	       m_next(p2) = m_next(p1);
	       noise("JOIN 3", p2);
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
	    m_next(p2) = p1;
	    noise("INSERT CHUNK", mem_chunk);
	    noise("FROM", p2);
	    chunk_list = p2;

	    if (p1 + m_size(p1) == m_next(p1))
	    {
	       if (p2 == m_next(p1))
		  chunk_list = p1;
	       m_size(p1) += m_size(m_next(p1));
	       m_next(p1) = m_next(m_next(p1));
	       noise("JOIN 4", p1);
	    }
	    return;
	 }
      }
      chunk_list = p2;		/* Save for search */
      p2 = m_next(p2);
   }
   while (p2 != chunk_list);

   /* If we get here we have a problem, ignore it, maybe it'll go away */
   noise("DROPPED CHUNK", mem_chunk);
}

/*
 * This function will search for a chunk in memory of at least 'mem_size'
 * when found, if the chunk is too big it'll be split, and pointer to the
 * chunk returned. If none is found NULL is returned.
 */

Static mem *
__search_chunk(mem_size)
unsigned int mem_size;
{
   register mem *p1, *p2;
   if (chunk_list == 0)		/* Simple case first */
      return 0;

   /* Search for a block >= the size we want */
   p1 = m_next(chunk_list);
   p2 = chunk_list;
   do
   {
      noise("CHECKED", p1);
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
      noise("FOUND RIGHT", p1);
      chunk_list = m_next(p2) = m_next(p1);
      if (chunk_list == p1)
	 chunk_list = 0;
      return p1;
   }

   noise("SPLIT", p1);
   /* Otherwise split it */
   m_next(p2) = p1 + mem_size;
   chunk_list = p2;

   p2 = m_next(p2);
   m_size(p2) = m_size(p1) - mem_size;
   m_next(p2) = m_next(p1);
   m_size(p1) = mem_size;
   if (chunk_list == p1)
      chunk_list = p2;
#ifdef VERBOSE
   p1[1].size = 0xAAAAAAAA;
#endif
   noise("INSERT CHUNK", p2);
   noise("FOUND CHUNK", p1);
   noise("LIST IS", chunk_list);
   return p1;
}

//-----------------------------------------------------------------------------

void *
calloc(elm, sz)
unsigned int elm, sz;
{
   register unsigned int v;
   register void *ptr;
   ptr = malloc(v = elm * sz);
   if (ptr)
      memset(ptr, 0, v);
   return ptr;
}

//------------------------------------------------------------------------------

void *
realloc(ptr, size)
void *ptr;
size_t size;
{
   void *nptr;
   unsigned int osize;
   if (ptr == 0)
      return malloc(size);

   osize = (m_size(((mem *) ptr) - 1) - 1) * sizeof(mem);
   if (size <= osize)
   {
      return ptr;
   }

   nptr = malloc(size);

   if (nptr == 0)
      return 0;

   memcpy(nptr, ptr, osize);
   free(ptr);

   return nptr;
}

//------------------------------------------------------------------------------
