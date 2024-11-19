/*
 * Small malloc/realloc/free with heap checking
 *	Ported to ELKS from V7 malloc by Greg Haerr 20 Apr 2020
 *
 * Enhancements:
 * Minimum 1024 bytes (BLOCK) allocated from kernel sbrk, > 1024 allocates requested size
 * Set DEBUG=1 for heap integrity checking on each call
 */
#include <unistd.h>
#include <stdlib.h>			/* __MINI_MALLOC must not be defined in malloc.h include*/
#define DEBUG		2		/* =1 heap checking asserts, =2 sysctl, =3 debug printf */

/*	C storage allocator
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of) 
 *	the next following block
 *	blocks are exact number of words long 
 *	aligned to the data type requirements of ALIGN
 *	pointers to blocks must have BUSY bit 0
 *	bit in ptr is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena (pointed to by alloct) is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
 *	a different implementation may need to redefine
 *	ALIGN, NALIGN, BLOCK, BUSY, INT
 *	where INT is integer type to which a pointer can be cast
 */
#define INT int
#define ALIGN int
#define NALIGN 1
#define WORD sizeof(union store)
//#define BLOCK 514	/* min+2, amount to sbrk*/
#define BLOCK 34	/* min+2, amount to sbrk*/
#define GRANULE 0	/* sbrk granularity*/
#define BUSY 1
#ifndef NULL
#define NULL 0
#endif
#define testbusy(p) ((INT)(p)&BUSY)
#define setbusy(p) (union store __wcnear *)((INT)(p)|BUSY)
#define clearbusy(p) (union store __wcnear *)((INT)(p)&~BUSY)

union store {
	union store __wcnear *ptr;
	ALIGN dummy[NALIGN];
	int calloc;	/*calloc clears an array of integers*/
};

static	union store allocs[2];	        /*initial arena*/
static	union store __wcnear *allocp;	/*search ptr*/
static	union store __wcnear *alloct;	/*arena top*/
static	union store __wcnear *allocx;	/*for benefit of realloc*/

#if DEBUG
#include <stdio.h>
#include <string.h>
#include <paths.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#define ASSERT(p)	if(!(p))malloc_assert_fail(#p);else
#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))
static void malloc_assert_fail(char *s);
static int malloc_check_heap(void);
#else
#define ASSERT(p)
#endif

#if DEBUG > 1
#define debug(...)	if (debug_level > 1) fprintf(dbgout, __VA_ARGS__)
static void malloc_show_heap(void);
static int debug_level = DEBUG;
static unsigned char bufdbg[64];
static FILE  dbgout[1] =
{
   {
    bufdbg,
    bufdbg,
    bufdbg,
    bufdbg,
    bufdbg + sizeof(bufdbg),
    -1,
    _IONBF | __MODE_WRITE | __MODE_IOTRAN
   }
};
#else
#define debug(...)
#define malloc_show_heap()
#endif

void *
malloc(size_t nbytes)
{
	union store __wcnear *p, __wcnear *q;
	int nw, temp;

#if DEBUG > 1
    if (dbgout->fd < 0)
        dbgout->fd = open(_PATH_CONSOLE, O_WRONLY);
#endif
#if DEBUG == 2
    sysctl(CTL_GET, "malloc.debug", &debug_level);
#endif

debug("(%d)malloc(%d) ", getpid(), nbytes);
	if (nbytes == 0)
		return NULL;	/* ANSI std */

	if(allocs[0].ptr==0) {	/*first time*/
		allocs[0].ptr = setbusy((union store __wcnear *)&allocs[1]);
		allocs[1].ptr = setbusy((union store __wcnear *)&allocs[0]);
		alloct = (union store __wcnear *)&allocs[1];
		allocp = (union store __wcnear *)&allocs[0];
	}
	nw = (nbytes+WORD+WORD-1)/WORD;			/* extra word for link ptr/size*/
	ASSERT(allocp>=allocs && allocp<=alloct);
	ASSERT(malloc_check_heap());
allocp = (union store __wcnear *)allocs;     /* experimental */
	debug("search start %p ", allocp);
	for(p=allocp; ; ) {
		for(temp=0; ; ) {
			if(!testbusy(p->ptr)) {
				while(!testbusy((q=p->ptr)->ptr)) {
					ASSERT(q>p);
					ASSERT(q<alloct);
					debug("(combine %u and %u) ",
						(char *)p->ptr - (char *)p, (char *)q->ptr - (char *)q);
					p->ptr = q->ptr;
				}
				if(q>=p+nw && p+nw>=p)
					goto found;
			}
			q = p;
			p = clearbusy(p->ptr);
			if(p>q)
				ASSERT(p<=alloct);
			else if(q!=alloct || p!=allocs) {
				ASSERT(q==alloct&&p==allocs);
				debug(" (corrupt) = NULL\n");
				return(NULL);
			} else if(++temp>1)
				break;
		}

		/* extend break at least BLOCK bytes at a time*/
		if (nw < BLOCK/WORD)
			temp = BLOCK/WORD;
		else
			temp = nw;

		debug("sbrk(%d) ", temp*WORD);
		/* ensure next sbrk returns even address*/
		q = (union store __wcnear *)sbrk(0);
		if((INT)q & (sizeof(union store) - 1))
			sbrk(4 - ((INT)q & (sizeof(union store) - 1)));

		/* check possible wrap (>= 32k alloc)*/
		if(q+temp+GRANULE < q) {
			debug(" (req too big) = NULL\n");
			return(NULL);
		}

		q = (union store __wcnear *)sbrk(temp*WORD);
		if((INT)q == -1) {
			debug(" (no more mem) = NULL\n");
			malloc_show_heap();
			return(NULL);
		}
		ASSERT(!((INT)q & 1));
		ASSERT(q>alloct);
		alloct->ptr = q;
		if(q!=alloct+1)			/* mark any gap as permanently allocated*/
			alloct->ptr = setbusy(alloct->ptr);
		alloct = q->ptr = q+temp-1;
debug("(TOTAL %u) ", 2+(char *)clearbusy(alloct) - (char *)clearbusy(allocs[1].ptr));
		alloct->ptr = setbusy(allocs);
	}
found:
	allocp = p + nw;
	ASSERT(allocp<=alloct);
	if(q>allocp) {
		allocx = allocp->ptr;	/* save contents in case of realloc data overwrite*/
		allocp->ptr = p->ptr;
	}
	p->ptr = setbusy(allocp);
debug("= %p\n", p);
malloc_show_heap();
	return((void *)(p+1));
}

/*	freeing strategy tuned for LIFO allocation
 */
void
free(void *ptr)
{
	union store __wcnear *p = (union store __wcnear *)ptr;

	if (p == NULL)
		return;
debug("(%d)free(%p)\n", getpid(), p-1);
	ASSERT(p>clearbusy(allocs[1].ptr)&&p<=alloct);
	ASSERT(malloc_check_heap());
	allocp = --p;
	ASSERT(testbusy(p->ptr));
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
malloc_show_heap();
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
 */
void *
realloc(void *ptr, size_t nbytes)
{
	union store __wcnear *p = (union store __wcnear *)ptr;
	union store __wcnear *q;
	union store __wcnear *s, __wcnear *t;
	unsigned int nw, onw;

	if (p == 0)
		return malloc(nbytes);
debug("(%d)realloc(%p,%d) ", getpid(), p-1, nbytes);

	ASSERT(testbusy(p[-1].ptr));
	if(testbusy(p[-1].ptr))
		free(p);
	onw = p[-1].ptr - p;
	q = (union store __wcnear *)malloc(nbytes);
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
		debug("allocx patch %p,%p,%d ", q, p, nw);
		(q+(q+nw-p))->ptr = allocx;
	}
debug("= %p\n", q);
	return((void *)q);
}

#if DEBUG
static void malloc_assert_fail(char *s)
{
	errmsg("malloc assert fail: ");
	errstr(s);
	abort();
}

static int
malloc_check_heap(void)
{
	union store __wcnear *p;
	int x = 0;

	for(p=(union store __wcnear *)&allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		if(p==allocp)
			x++;
	}
	if (p != alloct) debug("%p %p %p\n", p, alloct, p->ptr);
	ASSERT(p==alloct);
	return((x==1)|(p==allocp));
}
#endif

#if DEBUG > 1
static void
malloc_show_heap(void)
{
	union store __wcnear *p;
	int n = 1;
	unsigned int size, alloc = 0, free = 0;

	debug("--- heap size ---\n");
	malloc_check_heap();
	for(p = (union store __wcnear *)&allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		size = (char *)clearbusy(p->ptr) - (char *)clearbusy(p);
		debug("%2d: %p %4u", n, p, size);
		if (!testbusy(p->ptr)) {
			debug(" (free)");
			free += size;
		} else {
			if (n < 3)		/* don't count ptr to first sbrk()*/
				debug(" (skipped)");
			else alloc += size;
		}
		n++;
		debug("\n");
	}
	alloc += 2;
	debug("%2d: %p %4u  (top) alloc %u, free %u, total %u\n",
		n, alloct, 2, alloc, free, alloc+free);
}
#endif
