/*
 *	Memory management support for a swapping rather than paging kernel.
 *
 *	Memory allocator for ELKS. We keep a hole list so we can keep the
 *	malloc arena data in the kernel npot scattered into hard to read 
 *	user memory.
 */
 
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>

/*
 *	Worst case is code+data segments for max tasks unique processes
 *	and one for the free space left.
 */
 
#define MAX_SEGMENTS	8+(MAX_TASKS*2)

struct malloc_hole
{
	seg_t		page_base;	/* Pages */
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
	char * foo;
	while (m!=NULL && m->next!=NULL) {
		switch (m->flags) {
			case HOLE_SPARE:
				foo = "SPARE";
			break;
			case HOLE_FREE:
				foo = "FREE";
			break;
			case HOLE_USED:
				foo = "USED";
			break;
			default:
				foo = "DIDGEY";
			break;
		}
		printk("HOLE %x size %x is %s\n", m->page_base, m->extent, foo);
		m=m->next;
	}
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
			
int mm_alloc(pages)
segext_t pages;
{
	/*
	 *	Which hole fits best ?
	 */
	register struct malloc_hole *m=best_fit_hole(pages);
	/*
	 *	No room , later check priority and swap
	 */
	if(m==NULL) {
		return -1;
	}
	/*
	 *	The hole is (probably) too big
	 */
	 
	split_hole(m, pages);
	m->flags=HOLE_USED;
	m->refcount = 1;
	return m->page_base;
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
 * 	- any memory is preallocated via chmem */

int sys_brk(len)
__pptr len;
{
	register __ptask currentp = current;

	if (len < currentp->t_endtext || 
	    len > (currentp->t_endstack - USTACK_BYTES)) { 
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

