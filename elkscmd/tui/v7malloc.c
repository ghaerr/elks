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
#define DEBUG		1		/* =1 for heap checking asserts, =2 for debug printf*/

#if DEBUG
#include <stdio.h>
#define ASSERT(p)	if(!(p))botch(#p);else
static void botch(char *s);
static int allock(void);
void showheap(void);
#else
#define ASSERT(p)
#define showheap()
#endif

#if DEBUG > 1
#define debug		printf
#else
#define debug(str,...)
#endif

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
#define BLOCK 1028	/* 1024+4, amount to sbrk*/
#define GRANULE 0	/* sbrk granularity*/
#define BUSY 1
#ifndef NULL
#define NULL 0
#endif
#define testbusy(p) ((INT)(p)&BUSY)
#define setbusy(p) (union store *)((INT)(p)|BUSY)
#define clearbusy(p) (union store *)((INT)(p)&~BUSY)

union store {
	union store *ptr;
	ALIGN dummy[NALIGN];
	int calloc;	/*calloc clears an array of integers*/
};

static	union store allocs[2];	/*initial arena*/
static	union store *allocp;	/*search ptr*/
static	union store *alloct;	/*arena top*/
static	union store *allocx;	/*for benefit of realloc*/

void *
malloc(size_t nbytes)
{
	register union store *p, *q;
	int nw, temp;

debug("malloc(%d) %d ", getpid(), nbytes);
	if (nbytes == 0)
		return NULL;	/* ANSI std */

	if(allocs[0].ptr==0) {	/*first time*/
		allocs[0].ptr = setbusy(&allocs[1]);
		allocs[1].ptr = setbusy(&allocs[0]);
		alloct = &allocs[1];
		allocp = &allocs[0];
	}
	nw = (nbytes+WORD+WORD-1)/WORD;			/* extra word for link ptr/size*/
	ASSERT(allocp>=allocs && allocp<=alloct);
	ASSERT(allock());
	for(p=allocp; ; ) {
		for(temp=0; ; ) {
			if(!testbusy(p->ptr)) {
				while(!testbusy((q=p->ptr)->ptr)) {
					ASSERT(q>p);
					ASSERT(q<alloct);
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
				return(NULL);
			} else if(++temp>1)
				break;
		}

		/* extend break at least BLOCK bytes at a time*/
		if (nw < BLOCK/WORD)
			temp = BLOCK/WORD;
		else
			temp = nw;

		/* ensure next sbrk returns even address*/
		q = (union store *)sbrk(0);
		if((INT)q & (sizeof(union store) - 1))
			sbrk(4 - ((INT)q & (sizeof(union store) - 1)));

		/* check possible wrap (>= 32k alloc)*/
		if(q+temp+GRANULE < q) {
			ASSERT(q+temp+GRANULE>=q);
			return(NULL);
		}

		q = (union store *)sbrk(temp*WORD);
		if((INT)q == -1) {
			showheap();
			return(NULL);
		}
		ASSERT(!((INT)q & 1));
		ASSERT(q>alloct);
		alloct->ptr = q;
		if(q!=alloct+1)			/* mark any gap as permanently allocated*/
			alloct->ptr = setbusy(alloct->ptr);
		alloct = q->ptr = q+temp-1;
		debug("alloct->ptr %x, alloct %x\n", alloct->ptr, alloct);
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
debug("= %x\n", p);
	return((void *)(p+1));
}

/*	freeing strategy tuned for LIFO allocation
 */
void
free(void *ptr)
{
	register union store *p = (union store *)ptr;

	if (p == NULL)
		return;
debug("free(%d) %x\n", getpid(), p-1);
	ASSERT(p>clearbusy(allocs[1].ptr)&&p<=alloct);
	ASSERT(allock());
	allocp = --p;
	ASSERT(testbusy(p->ptr));
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
 */
void *
realloc(void *ptr, size_t nbytes)
{
	register union store *p = ptr;
	register union store *q;
	union store *s, *t;
	register unsigned nw;
	unsigned onw;

	if (p == 0)
		return malloc(nbytes);
debug("realloc(%d) %x %d ", getpid(), p-1, nbytes);

	ASSERT(testbusy(p[-1].ptr));
	if(testbusy(p[-1].ptr))
		free(p);
	onw = p[-1].ptr - p;
	q = (union store *)malloc(nbytes);
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
debug("realloc patch with allocx %x,%x,%d\n", q, p, nw);
		(q+(q+nw-p))->ptr = allocx;
	}
debug("= %x\n", q);
	return((void *)q);
}

#if DEBUG
static void botch(char *s)
{
	printf("malloc assert fail: %s\n",s);
	abort();
}

static int
allock(void)
{
	register union store *p;
	int x;
	x = 0;
	//printf("[(%x),", (int)alloct);
	for(p=&allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		//printf("%x,", (int)p);
		if(p==allocp)
			x++;
	}
	//printf("]\n");
	if (p != alloct) printf("%x %x %x\n", (int)p, (int)alloct, (int)p->ptr);
	ASSERT(p==alloct);
	return((x==1)|(p==allocp));
}

void
showheap(void)
{
	register union store *p;
	int n = 1;
	int size, alloc = 0, free = 0;

	printf("HEAP LIST\n");
	allock();
	for(p=&allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		size = (int)clearbusy(p->ptr) - (int)clearbusy(p) - 2;
		printf("%d: %04x size %d", n, (int)p+2, size);
		if (!testbusy(p->ptr)) {
			printf(" (free)");
			free += size;
		} else {
			if (n < 3)		/* don't count ptr to first sbrk()*/
				printf(" (skipped)");
			else alloc += size + 2;
		}
		n++;
		printf("\n");
	}
	printf("%d: %x (top)\n", n, (int)alloct);
	printf("Total alloc %u, free %u\n", alloc, free);
}
#endif
