/*
 * _dmalloc - Small malloc/realloc/free with heap checking
 *  Ported to ELKS from V7 malloc by Greg Haerr 20 Apr 2020
 *
 * Enhancements:
 * Minimum BLOCK allocate from kernel sbrk, > BLOCK allocates requested size
 * Much improved size and heap overflow handling with errno returns
 * Full heap integrity checking and reporting with debug options
 * Use near heap pointers to work with OpenWatcom large model
 * Combine free areas at heap start before allocating from free area at end of heap
 */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysctl.h>
#ifdef __C86__
#define DEBUG 0
#else
#define DEBUG       1       /* =1 use sysctl, =2 debug output, =3 show heap */
#endif

/*  C storage allocator
 *  circular first-fit strategy
 *  works with noncontiguous, but monotonically linked, arena
 *  each block is preceded by a ptr to the (pointer of) 
 *  the next following block
 *  blocks are exact number of words long 
 *  aligned to the data type requirements of ALIGN
 *  pointers to blocks must have BUSY bit 0
 *  bit in ptr is 1 for busy, 0 for idle
 *  gaps in arena are merely noted as busy blocks
 *  last block of arena (pointed to by alloct) is empty and
 *  has a pointer to first
 *  idle blocks are coalesced during space search
 *
 *  a different implementation may need to redefine
 *  ALIGN, NALIGN, BLOCK, BUSY, INT
 *  where INT is integer type to which a pointer can be cast
 */
#define INT int
#define ALIGN int
#define NALIGN 1
#define BUSY 1
#define BLOCK       34              /* min+WORD amount to sbrk (was 514) */
#define MINALLOC    14              /* minimum actual malloc size */
#define GRANULE     0               /* sbrk granularity */

union store {
    union store __wcnear *ptr;
    ALIGN dummy[NALIGN];
};
typedef union store __wcnear *NPTR;
#define WORD            sizeof(union store)

#define testbusy(p)           ((INT)(p)&BUSY)
#define setbusy(p)      (NPTR)((INT)(p)|BUSY)
#define clearbusy(p)    (NPTR)((INT)(p)&~BUSY)
#define next(p)         ((p)->ptr)

#define SIZE     2
static  union store allocs[SIZE];
static  NPTR allocp;   /*search ptr*/
static  NPTR alloct;   /*arena top*/
static  NPTR allocx;   /*for benefit of realloc*/

static int debug_level = DEBUG;
#if DEBUG
#define ASSERT(p)   if(!(p))malloc_assert_fail(#p,__LINE__);else {}
#define debug(...)  do { if (debug_level > 1) __dprintf(__VA_ARGS__); } while (0)
#define debug2(...) do { if (debug_level > 2) __dprintf(__VA_ARGS__); } while (0)
static void malloc_assert_fail(char *s, int);
static void malloc_show_heap(void);
static int malloc_check_heap(void);
#else
#define ASSERT(p)
#define debug(...)
#define debug2(...)
#define malloc_show_heap()
#endif

void *
_dmalloc(size_t nbytes)
{
    NPTR p, q;
    unsigned int nw, temp;

#if DEBUG == 1
    if (debug_level == 1) sysctl(CTL_GET, "malloc.debug", &debug_level);
#endif
    if (allocs[0].ptr == 0) {  /*first time*/
        allocs[0].ptr = setbusy(&allocs[1]);
        allocs[SIZE-1].ptr = setbusy(&allocs[0]);
        alloct = (NPTR)&allocs[SIZE-1];
        allocp = (NPTR)&allocs[0];
    }

    debug("(%d)malloc(%5u) ", getpid(), nbytes);
    errno = 0;
    if (nbytes == 0) {
        debug(" (malloc 0) = NULL\n");
        return NULL;        /* ANSI std, no error */
    }
    if (nbytes < MINALLOC)
        nbytes = MINALLOC;

    /* check INT overflow beyond 32762 (nbytes/WORD+2*WORD+(WORD-1) > 0xFFFF/WORD/WORD) */
    if (nbytes > ((unsigned)-1)/WORD-2*WORD-(WORD-1)) {
        debug(" (req too big) = NULL\n");
        errno = ENOMEM;
        return(NULL);
    }
    nw = (nbytes+WORD+WORD-1)/WORD;          /* extra word for link ptr/size*/

    ASSERT(allocp>=allocs && allocp<=alloct);
    ASSERT(malloc_check_heap());
    /* combine free areas at heap start before allocating from free area past allocp */
    allocp = (NPTR)allocs;
    for(p=allocp; ; ) {
        for(temp=0; ; ) {
            if(!testbusy(next(p))) {
                while(!testbusy(next(q = next(p)))) {
                    if (debug_level > 2) malloc_show_heap();
                    ASSERT(q>p);
                    ASSERT(q<alloct);
                    debug2("(combine %u and %u) ",
                        (next(p) - p) * sizeof(union store),
                        (next(q) - q) * sizeof(union store));
                    next(p) = next(q);
                }
                /*debug2("q %04x p %04x nw %d p+nw %04x ", (unsigned)q, (unsigned)p,
                    nw, (unsigned)(p+nw));*/
                if(q>=p+nw && p+nw>=p)
                    goto found;
            }
            q = p;
            p = clearbusy(next(p));
            if(p>q) {
                ASSERT(p<=alloct);
            } else if(q!=alloct || p!=(NPTR)allocs) {
                ASSERT(q==alloct&&p==(NPTR)allocs);
                debug(" (corrupt) = NULL\n");
                errno = ENOMEM;
                return(NULL);
            } else if(++temp>1)
                break;
        }

        /* extend break at least BLOCK bytes at a time, plus a word for top link */
        if (nw < BLOCK/WORD)
            temp = BLOCK/WORD + 1;
        else
            temp = nw + 1; /* NOTE always allocates full req w/o looking at free at top */

        if (debug_level > 2) malloc_show_heap();
        debug2("sbrk(%d) ", temp*WORD);
#if 0   /* not required and slow, initial break always even */
        q = (NPTR)sbrk(0);
        if((INT)q & (sizeof(union store) - 1))
            sbrk(4 - ((INT)q & (sizeof(union store) - 1)));

        /* check possible address wrap - performed in kernel */
        if(q+temp+GRANULE < q) {
            debug(" (no more address space) = NULL\n");
            errno = ENOMEM;
            return(NULL);
        }
#endif
        q = (NPTR)sbrk(temp*WORD);
        if((INT)q == -1) {
            debug(" (no more mem) = NULL\n");
            malloc_show_heap();
            errno = ENOMEM;
            return(NULL);
        }
        ASSERT(!((INT)q & 1));
        ASSERT(q>alloct);
        next(alloct) = q;
        if(q!=alloct+1)         /* mark any gap as permanently allocated*/
            next(alloct) = setbusy(next(alloct));
        alloct = next(q) = q+temp-1;
        debug2("(TOTAL %u) ",
            sizeof(union store) +
            (clearbusy(alloct) - clearbusy(allocs[SIZE-1].ptr)) * sizeof(union store));
        next(alloct) = setbusy(allocs);
    }
found:
    allocp = p + nw;
    ASSERT(allocp<=alloct);
    if(q>allocp) {
        allocx = next(allocp);   /* save contents in case of realloc data overwrite*/
        next(allocp) = next(p);
    }
    next(p) = setbusy(allocp);
    debug2("= %04x\n", (unsigned)p);
    malloc_show_heap();
    return((void *)(p+1));
}

/*  freeing strategy tuned for LIFO allocation
 */
void
_dfree(void *ptr)
{
    NPTR p = (NPTR)ptr;

    if (ptr == NULL)
        return;
    debug("(%d)  free(%5u) ", getpid(), (unsigned)(next(p-1) - p) * sizeof(union store));
    debug2("= %04x\n", p-1);
    ASSERT(p>clearbusy(allocs[SIZE-1].ptr)&&p<=alloct);
    ASSERT(malloc_check_heap());
    allocp = --p;
    ASSERT(testbusy(next(p)));
    next(p) = clearbusy(next(p));
    ASSERT(next(p) > allocp && next(p) <= alloct);
    malloc_show_heap();
}

size_t _dmalloc_usable_size(void *ptr)
{
    NPTR p = (NPTR)ptr;

    if (p == NULL)
        return 0;
    ASSERT(p>clearbusy(allocs[SIZE-1].ptr)&&p<=alloct);
    --p;
    ASSERT(testbusy(next(p)));
    return (clearbusy(next(p)) - clearbusy(p)) * sizeof(union store);
}

/*  realloc(p, nbytes) reallocates a block obtained from malloc()
 *  and freed since last call of malloc()
 *  to have new size nbytes, and old content
 *  returns new location, or 0 on failure
 */
void *
_drealloc(void *ptr, size_t nbytes)
{
    NPTR p = (NPTR)ptr;
    NPTR q;
    NPTR s, t;
    unsigned int nw, onw;

    if (p == 0)
        return _dmalloc(nbytes);
    debug("(%d)realloc(%04x,%u) ", getpid(), (unsigned)(p-1), nbytes);

    ASSERT(testbusy(next(p-1)));
    if(testbusy(next(p-1)))
        _dfree(p);
    onw = next(p-1) - p;
    q = (NPTR)_dmalloc(nbytes);   // FIXME and also use memcpy
    if(q==NULL || q==p)
        return((void *)q);

    /* copy old data into new allocation*/
    s = p;
    t = q;
    nw = (nbytes+WORD-1)/WORD;
    if(nw<onw)
        onw = nw;
    while(onw--!=0)
        *t++ = *s++;

    /* restore old data for special case of malloc link overwrite*/
    if(q<p && q+nw>=p) {
        debug("allocx patch %04x,%04x,%d ", (unsigned)q, (unsigned)p, nw);
        next(q+(q+nw-p)) = allocx;
    }
    debug("= %04x\n", (unsigned)q);
    return((void *)q);
}

#if DEBUG
static void malloc_assert_fail(char *s, int line)
{
    __dprintf("dmalloc assert fail: %s (line %d)\n", s, line);
    abort();
}

static int
malloc_check_heap(void)
{
    NPTR p;
    int x = 0;

    for(p=(NPTR)&allocs[0]; clearbusy(next(p)) > p; p=clearbusy(next(p))) {
        if(p==allocp)
            x++;
    }
    if (p != alloct) debug("%04x %04x %04x\n",
        (unsigned)p, (unsigned)alloct, (unsigned)next(p));
    ASSERT(p==alloct);
    return((x==1)|(p==allocp));
}

static void
malloc_show_heap(void)
{
    NPTR p;
    int n = 1;
    unsigned int size, alloc = 0, free = 0;
    static unsigned int maxalloc;

    if (debug_level < 2) return;
    debug2("--- heap size ---\n");
    malloc_check_heap();
    for(p = (NPTR)&allocs[0]; clearbusy(next(p)) > p; p=clearbusy(next(p))) {
        size = (clearbusy(next(p)) - clearbusy(p)) * sizeof(union store);
        debug2("%2d: %04x %4u", n, (unsigned)p, size);
        if (!testbusy(next(p))) {
            debug2(" (free)");
            free += size;
        } else {
            if (n < 3)      /* don't count ptr to first sbrk()*/
                debug2(" (skipped)");
            else
                alloc += size;
        }
        n++;
        debug2("\n");
    }
    alloc += sizeof(union store);
    if (alloc > maxalloc) maxalloc = alloc;
    debug2("%2d: %04x %4u (top) ", n, (unsigned)alloct, 2);
    debug("alloc %5u, free %5u, arena %5u/%5u\n", alloc, free, maxalloc, alloc+free);
}
#endif
