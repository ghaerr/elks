/*
 * __amalloc - Arena-based heap allocator - provides up to 64k local (far) heap
 * Based on __dmalloc (v7 debug malloc).
 * 16 Dec 2024 Greg Haerr
 *
 * Small malloc/realloc/free with heap checking
 *  Ported to ELKS from V7 malloc by Greg Haerr 20 Apr 2020
 *
 * Enhancements:
 * Minimum BLOCK allocate from kernel sbrk, > BLOCK allocates requested size
 * Much improved size and heap overflow handling with errno returns
 * Full heap integrity checking and reporting with DEBUG options
 * Use near heap pointers to work with OpenWatcom large model
 * Combine free areas at heap start before allocating from free area at end of heap
 */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysctl.h>
#define DEBUG       2       /* =1 heap checking asserts, =2 sysctl, =3 show heap */

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
#define BLOCK       34              /* min+WORD amount to sbrk */
#define MINALLOC    14              /* minimum actual malloc size */
#define GRANULE     0               /* sbrk granularity */

union store {
    union store __wcnear *ptr;
    ALIGN dummy[NALIGN];
};
typedef union store __wcnear *NPTR;
typedef union store __far    *FPTR;
#define WORD            sizeof(union store)

#define FP_SEG(fp)       ((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define FP_OFF(fp)       ((unsigned)(unsigned long)(void __far *)(fp))
#define MK_FPTR(seg,off) ((FPTR)((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

#define testbusy(p)           ((INT)(p)&BUSY)
#define setbusy(p)      (NPTR)((INT)(p)|BUSY)
#define clearbusy(p)    (NPTR)((INT)(p)&~BUSY)
#define next(p)         ((MK_FPTR(allocseg,p))->ptr)

static FPTR allocs;             /* arena base address */
static unsigned int allocsize;  /* total arena size in bytes */
static unsigned int allocseg;   /* arena segment */

static  NPTR allocp;   /*search ptr*/
static  NPTR alloct;   /*arena top*/
static  NPTR allocx;   /*for benefit of realloc*/

#if DEBUG
#define ASSERT(p)   if(!(p))malloc_assert_fail(#p);else {}
static void malloc_assert_fail(char *s);
static int malloc_check_heap(void);
#else
#define ASSERT(p)
#endif

#if DEBUG > 1
#define debug(...)  do { if (debug_level > 1) __dprintf(__VA_ARGS__); } while (0)
#define debug2(...) do { if (debug_level > 2) __dprintf(__VA_ARGS__); } while (0)
int __dprintf(const char *fmt, ...);
static void malloc_show_heap(void);
static int debug_level = DEBUG;
#else
#define debug(...)
#define malloc_show_heap()
#endif

/* add size bytes to arena malloc heap, must be done before first malloc */
int __amalloc_add_heap(char __far *start, size_t size)
{
    ASSERT(start != NULL && size >= 16);
    allocs = (FPTR)start;
    allocseg = FP_SEG(start);
    allocsize = size / sizeof(union store);
    debug("Adding %04x %04x size %d DS %04x %04x\n", start, size, &size);

    allocs[0].ptr = setbusy(&allocs[1]);
    allocs[1].ptr = (NPTR)&allocs[allocsize-2];
    allocs[allocsize-2].ptr = setbusy(&allocs[allocsize-1]);
    allocs[allocsize-1].ptr = setbusy(&allocs[0]);
    alloct = (NPTR)&allocs[allocsize-1];
    allocp = (NPTR)&allocs[0];
    return 1;
}

void *
__amalloc(size_t nbytes)
{
    NPTR p, q;
    unsigned int nw, temp;

#if DEBUG == 2
    sysctl(CTL_GET, "malloc.debug", &debug_level);
#endif

    debug("(%d)malloc(%u) ", getpid(), nbytes);
    if (!allocs)
        return NULL;
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
                    debug("(combine %u and %u) ",
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

#if 1 // SIZE > 2
        debug("Out of fixed heap\n");
        return NULL;
#else

        /* extend break at least BLOCK bytes at a time, plus a word for top link */
        if (nw < BLOCK/WORD)
            temp = BLOCK/WORD + 1;
        else
            temp = nw + 1; /* NOTE always allocates full req w/o looking at free at top */

        if (debug_level > 2) malloc_show_heap();
        debug("sbrk(%d) ", temp*WORD);
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
        debug("(TOTAL %u) ",
            sizeof(union store) +
            (clearbusy(alloct) - clearbusy(allocs[allocsize-1].ptr)) * sizeof(union store));
        next(alloct) = setbusy(allocs);
#endif
    }
found:
    allocp = p + nw;
    ASSERT(allocp<=alloct);
    if(q>allocp) {
        allocx = next(allocp);   /* save contents in case of realloc data overwrite*/
        next(allocp) = next(p);
    }
    next(p) = setbusy(allocp);
    debug("= %04x\n", (unsigned)p);
    malloc_show_heap();
    return MK_FPTR(allocseg, p+1);
}

/*  freeing strategy tuned for LIFO allocation
 */
void
__afree(void *ptr)
{
    NPTR p = (NPTR)ptr;

    if (p == NULL)
        return;
    debug("(%d)  free(%d) = %04x\n", getpid(),
        (unsigned)(next(p-1) - p) * sizeof(union store), p-1);
    ASSERT(FP_SEG(ptr)==allocseg);
    ASSERT(p>clearbusy(allocs[allocsize-1].ptr)&&p<=alloct);
    ASSERT(malloc_check_heap());
    allocp = --p;
    ASSERT(testbusy(next(p)));
    next(p) = clearbusy(next(p));
    ASSERT(next(p) > allocp && next(p) <= alloct);
    malloc_show_heap();
}

size_t __amalloc_usable_size(void *ptr)
{
    NPTR p = (NPTR)ptr;

    if (ptr == NULL)
        return 0;
    ASSERT(FP_SEG(ptr)==allocseg);
    ASSERT(p>clearbusy(allocs[allocsize-1].ptr)&&p<=alloct);
    --p;
    ASSERT(testbusy(next(p)));
    return (clearbusy(next(p)) - clearbusy(p)) * sizeof(union store);
}

#if LATER
/*  realloc(p, nbytes) reallocates a block obtained from malloc()
 *  and freed since last call of malloc()
 *  to have new size nbytes, and old content
 *  returns new location, or 0 on failure
 */
void *
__arealloc(void *ptr, size_t nbytes)
{
    NPTR p = (NPTR)ptr;
    NPTR q;
    NPTR s, t;
    unsigned int nw, onw;

    if (p == 0)
        return __amalloc(nbytes);
    debug("(%d)realloc(%04x,%u) ", getpid(), (unsigned)(p-1), nbytes);

    ASSERT(testbusy(next(p-1)));
    if(testbusy(next(p-1)))
        __afree(p);
    onw = next(p-1) - p;
    q = (NPTR)__amalloc(nbytes);   // FIXME and also use memcpy
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
#endif

#if DEBUG
static void malloc_assert_fail(char *s)
{
    __dprintf("malloc assert fail: %s\n", s);
    abort();
}

static int
malloc_check_heap(void)
{
    NPTR p;
    int x = 0;

    //debug("next(0) = %04x\n", clearbusy(next(&allocs[0])));
    for(p=(NPTR)&allocs[0]; clearbusy(next(p)) > p; p=clearbusy(next(p))) {
        //debug("%04x %04x %04x\n", (unsigned)p, (unsigned)alloct, (unsigned)next(p));
        if(p==allocp)
            x++;
    }
    if (p != alloct) debug("%04x %04x %04x\n",
        (unsigned)p, (unsigned)alloct, (unsigned)next(p));
    ASSERT(p==alloct);
    return((x==1)|(p==allocp));
}
#endif

#if DEBUG > 1
static void
malloc_show_heap(void)
{
    NPTR p;
    int n = 1;
    unsigned int size, alloc = 0, free = 0;

    debug2("--- heap size ---\n");
    malloc_check_heap();
    for(p = (NPTR)&allocs[0]; clearbusy(next(p)) > p; p=clearbusy(next(p))) {
        size = (clearbusy(next(p)) - clearbusy(p)) * sizeof(union store);
        debug2("%2d: %04x %4u", n, (unsigned)p, size);
        if (!testbusy(next(p))) {
            debug2(" (free)");
            free += size;
        } else {
            if (n < 2)
                debug2(" (skipped)");
            alloc += size;
        }
        n++;
        debug2("\n");
    }
    alloc += sizeof(union store);
    debug2("%2d: %04x %4u (top) ", n, (unsigned)alloct, 2);
    debug("alloc %u, free %u, total %u\n", alloc, free, alloc+free);
}
#endif
