/*
 *	Memory management support for a swapping rather than paging kernel.
 *
 *	Memory allocator for ELKS. We keep a hole list so we can keep the
 *	malloc arena data in the kernel npot scattered into hard to read 
 *	user memory.
 *
 *	20th Jan 2000	Alistair Riddoch (ajr@ecs.soton.ac.uk)
 *			For reasons explained in fs/exec.c, support is
 *			required for holes which do not start at the
 *			the bottom of the segment. To facilitate this
 *			I have added a hole_seg field to the hole descriptor
 *			structure which indicates the segment number the hole
 *			exists in. Later we will add support for growing holes
 *			in both directions to allow for a growing stack at the
 *			bottom and a heap at the top.
 */
 
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>

/*
 *	Worst case is code+data segments for max tasks unique processes
 *	and one for the free space left.
 */
 
#define MAX_SEGMENTS	8+(MAX_TASKS*2)

/* This option specifies whether we have holes that are in a segment not equal
 * to the page base. This is require for binaries with the stack at the
 * bottom */
/* #define FLOAT_HOLES /**/

struct malloc_hole
{
	seg_t		page_base;	/* Pages */
#ifdef FLOAT_HOLES
	seg_t		hole_seg;	/* segment */
#endif
	segext_t	extent;		/* Pages */
	struct malloc_hole *next;	/* Next in list memory order */
	int		refcount;
	int		flags;		/* So we know if it is free */
#define HOLE_USED		1	
#define HOLE_FREE		2
#define HOLE_SPARE		3
};

static struct malloc_hole holes[MAX_SEGMENTS];

/*
 *	Find a spare hole.
 */
 
struct malloc_hole *alloc_hole()
{
	int ct=0;
	register struct malloc_hole *m=&holes[0];

	while(ct<MAX_SEGMENTS) {
		if(m->flags==HOLE_SPARE) {
			return m;
		}
		m++;
		ct++;
	}
	panic("mm: too many holes");
}

/*
 *	Split a hole into two
 */
 
static void split_hole(m, len)
register struct malloc_hole *m;
segext_t len;
{
	register struct malloc_hole *n;
	seg_t spare=m->extent-len;

	if (!spare)
		return;
	/*
	 *	Split into one allocated one free
	 */
	 
	m->extent=len;
	n=alloc_hole();
	n->page_base=m->page_base+len;
	n->extent=spare;
	n->next=m->next;
	m->next=n;
	n->flags=HOLE_FREE;
}

/*
 *	Merge adjacent free holes
 */
 
static void sweep_holes()
{
	register struct malloc_hole *m=&holes[0];

	while(m!=NULL && m->next!=NULL)
	{
		if(m->flags==HOLE_FREE && m->next->flags==HOLE_FREE && 
			m->page_base+m->extent==m->next->page_base)
		{
			m->extent+=m->next->extent;
			m->next->flags=HOLE_SPARE;
			m->next=m->next->next;
		}
		else
			m=m->next;
	}
}
#if 0
void dmem()
{
	register struct malloc_hole *m=&holes[0];
	char * status;
	do {
		switch (m->flags) {
			case HOLE_SPARE:
				status = "SPARE";
				break;
			case HOLE_FREE:
				status = "FREE";
				break;
			case HOLE_USED:
				status = "USED";
				break;
			default:
				status = "DIDGEY";
				break;
		}
		printk("HOLE %x size %x is %s\n", m->page_base, m->extent, status);
		m=m->next;
	} while (m!=NULL);
}
#endif
	

/*
 *	Find the nearest fitting hole
 */
 
static struct malloc_hole *best_fit_hole(size)
segext_t size;
{
	register struct malloc_hole *m=&holes[0];
	register struct malloc_hole *best=NULL;

	while (m!=NULL) {
		if (m->extent>=size && m->flags==HOLE_FREE) {
			if(!best || best->extent > m->extent) {
				best=m;
			}
		}
		m=m->next;
	}
	return best;
}

/*
 *	Find the hole starting at a given location
 */
 
static struct malloc_hole *find_hole(base)
seg_t base;
{
	register struct malloc_hole *m=&holes[0];

	while (m!=NULL) {
		if(m->page_base==base)
			return m;
		m=m->next;
	}
	panic("arena corrupt");
	return NULL;
}		

/*
 *	Allocate a segment
 */
			
#ifdef FLOAT_HOLES
seg_t mm_alloc(pages, offset)
segext_t pages;
segext_t offset;
#else
seg_t mm_alloc(pages)
segext_t pages;
#endif
{
	/*
	 *	Which hole fits best ?
	 */
	register struct malloc_hole *m=best_fit_hole(pages);
	/*
	 *	No room , later check priority and swap
	 */
	if(m==NULL) {
		return NULL;
	}
	/*
	 *	The hole is (probably) too big
	 */
	 
	split_hole(m, pages);
	m->flags=HOLE_USED;
	m->refcount = 1;
#ifdef FLOAT_HOLES
	m->hole_seg = m->page_base - offset;
	/* FIXME must check this is legal */
	return m->hole_seg;
#else
	return m->page_base;
#endif
}

/* 	Increase refcount */

seg_t mm_realloc(base)
seg_t base;
{
	register struct malloc_hole *m=find_hole(base);

	m->refcount++;
	return m->page_base;
}

/*
 *	Free a segment.
 */
 
void mm_free(base)
seg_t base;
{
	register struct malloc_hole *m=find_hole(base);

	if (m->flags!=HOLE_USED) {
		panic("double free");
	}
	m->refcount--;
	if (!m->refcount) {
		m->flags=HOLE_FREE;
		sweep_holes();
	}
}

/*
 *	Allocate a segment and copy it from another
 */
 
seg_t mm_dup(base)
seg_t base;
{
	register struct malloc_hole *o, *m;
	size_t i;
	
	o=find_hole(base);
	m=best_fit_hole(o->extent);
	if(m==NULL) {
		return NULL;
	}
	split_hole(m, o->extent);
	m->flags=HOLE_USED;
	m->refcount = 1;
	i = (o->extent << 4)/* - 2*/;
	fmemcpy(m->page_base, 0, o->page_base, 0, i);
	return m->page_base;
}

/*	This is just to keep malloc et. al happy - it doesn't really do anything
 * 	- any memory is preallocated via chmem
 */

/* If the stack is at the top of the data segment then we have to leave
 * room for it, otherwise we just need a safety margin of 0x100 bytes
 * AJR <ajr@ecs.soton.ac.uk> 27/01/2000
 */
#define HEAP_LIMIT ((currentp->t_begstack > currentp->t_enddata) ? \
			USTACK_BYTES : 0x100)


int sys_brk(len)
__pptr len;
{
	register __ptask currentp = current;

	if (len < currentp->t_enddata || 
	    (len > (currentp->t_endseg - HEAP_LIMIT))) {
		return -ENOMEM; 
	}

	currentp->t_endbrk = len;
	return 0;
}

/*
 *	Initialise the memory manager.
 */

void mm_init(start,end)
seg_t start;
seg_t end;
{
	int ct;
	register struct malloc_hole * holep = &holes[0];
	/*
	 *	Mark pages free.
	 */
	for(ct=1;ct<MAX_SEGMENTS;ct++)
		holes[ct].flags=HOLE_SPARE;
	/*
	 *	Single hole containing all user memory.
	 */
	holep->flags=HOLE_FREE;
	holep->page_base = start;
	holep->extent = end-start;
	holep->refcount = 0;
	holep->next = NULL;
}

