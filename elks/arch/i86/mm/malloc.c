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
#define MAX_SWAP_SEGMENTS	8+(MAX_TASKS*2)

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
	__u8		refcount;
	__u8		flags;		/* So we know if it is free */
#define HOLE_USED		1	
#define HOLE_FREE		2
#define HOLE_SPARE		3
#define HOLE_SWAPPED		8
};

static struct malloc_hole holes[MAX_SEGMENTS];
static struct malloc_hole swap_holes[MAX_SWAP_SEGMENTS];

struct malloc_head
{
	struct malloc_hole *holes;
	int size;
};

struct malloc_head memmap = { holes, MAX_SEGMENTS };
struct malloc_head swapmap = { swap_holes, MAX_SWAP_SEGMENTS };

/*
 *	Find a spare hole.
 */
 
static struct malloc_hole *alloc_hole(mh)
struct malloc_head *mh;
{
	int ct=0;
	register struct malloc_hole *m=mh->holes;

	while(ct<mh->size) {
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
 
static void split_hole(mh, m, len)
struct malloc_head *mh;
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
	n=alloc_hole(mh);
	n->page_base=m->page_base+len;
	n->extent=spare;
	n->next=m->next;
	m->next=n;
	n->flags=HOLE_FREE;
}

/*
 *	Merge adjacent free holes
 */
 
static void sweep_holes(mh)
struct malloc_head *mh;
{
	register struct malloc_hole *m=mh->holes;

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
void dmem(mh)
struct malloc_head *mh;
{
	register struct malloc_hole *m=mh->holes;
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
				status = "DODGY";
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
 
static struct malloc_hole *best_fit_hole(mh, size)
struct malloc_head *mh;
segext_t size;
{
	register struct malloc_hole *m=mh->holes;
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
 
static struct malloc_hole *find_hole(mh, base)
struct malloc_head *mh;
seg_t base;
{
	register struct malloc_hole *m=mh->holes;

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
	register struct malloc_hole *m=best_fit_hole(&memmap, pages);
	/*
	 *	No room , later check priority and swap
	 */
	if(m==NULL) {
		return NULL;
	}
	/*
	 *	The hole is (probably) too big
	 */
	 
	split_hole(&memmap, m, pages);
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
	register struct malloc_hole *m=find_hole(&memmap, base);

	m->refcount++;
	return m->page_base;
}

/*
 *	Free a segment.
 *
 *	Caution: A process can be killed dead by another. That means we
 *	might potentially have to kill a swapped task. Be sure to catch
 *	that in exit...
 */
 
void mm_free(base)
seg_t base;
{
	register struct malloc_hole *m=find_hole(&memmap, base);

	if ((m->flags&3)!=HOLE_USED) {
		panic("double free");
	}
	m->refcount--;
	if (!m->refcount) {
		m->flags=HOLE_FREE;
		sweep_holes(&memmap);
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
	
	o=find_hole(&memmap,base);
	if(o->flags!=HOLE_USED)
		panic("bad/swapped hole");
	m=best_fit_hole(&memmap, o->extent);
	if(m==NULL) {
		return NULL;
	}
	split_hole(&memmap, m, o->extent);
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

/*
 *	Swapper task
 */
 
static struct buffer_head swap_buf;
static dev_t swap_dev;

/*
 *	Push a segment to disk if possible
 */
 
static int swap_out(base)
seg_t base;
{
	register struct task_struct *t;
	register struct malloc_hole *o=find_hole(&memmap,base);
	struct malloc_hole *so;
	int ct, blocks;
	/* We can hit disk this time. Allocate a hole in 1K increments */
	blocks = (o->extent+0x03FF)>>10;
	so = best_fit_hole(&swapmap, blocks);
	if(so==NULL)
	{
		/* No free swap */
		return -1;
	}
	split_hole(&swapmap, so, blocks);
	so->flags=HOLE_USED;
	so->refcount=o->refcount;

	for_each_task(t)
	{
		int c=t->mm.flags;
		if(t->mm.cseg == base &&  !(c&CS_SWAP))
		{
			t->mm.cseg = so->page_base;
			t->mm.flags |= CS_SWAP;
		}
		if(t->mm.dseg == base &&  !(c&DS_SWAP))
		{
			t->mm.cseg = so->page_base;
			t->mm.flags |= DS_SWAP;
		}
		/* If we are running and we are now swapped and we were not
		   swapped before.. then remove us from the run queue */
		   
		if(c && !t->mm.flags && t->state == TASK_RUNNING)
			del_from_runqueue(t);
	}

	/* Now write the segment out */

	for(ct=0; ct< blocks; ct++)
	{
		swap_buf.b_blocknr = (so->page_base+ct)<<1;
		swap_buf.b_dev = swap_dev;
		swap_buf.b_lock = 0;
		swap_buf.b_dirty = 1;
		swap_buf.b_seg = o->page_base;
		swap_buf.b_data = ct<<10;
		ll_rw_blk(WRITE, &swap_buf);
		wait_on_buffer(&swap_buf);
	}	
	return 1;
}

static int swap_in(base, chint)
seg_t base;
int chint;
{
	register struct malloc_hole *o;
	struct malloc_hole *so=find_hole(&swapmap,base);
	int ct, blocks;
	register struct task_struct *t;

	/* Find memory for this segment */
	o=best_fit_hole(&memmap, so->extent<<10);
	if(o==NULL)
		return -1;
	
	/* Now read the segment in */
	split_hole(&memmap, so, so->extent<<10);
	o->flags = HOLE_USED;
	o->refcount = so->refcount;
	
	blocks = so->extent;
	
	for(ct=0; ct< blocks; ct++)
	{
		swap_buf.b_blocknr = (so->page_base+ct)<<1;
		swap_buf.b_dev = swap_dev;
		swap_buf.b_lock = 0;
		swap_buf.b_dirty = 0;
		swap_buf.b_seg = o->page_base;
		swap_buf.b_data = ct<<10;
		/*
		 *	We want probably 4 of these if we do real I/O
		 *	later on.
		 */
		ll_rw_blk(READ, &swap_buf);
		wait_on_buffer(&swap_buf);
	}	
	
	/*
	 *	Update the memory management tables
	 */
	 
	for_each_task(t)
	{
		int c = t->mm.flags;
		if(t->mm.cseg == base &&  c&CS_SWAP)
		{
			t->mm.cseg = o->page_base;
			t->mm.flags &= ~CS_SWAP;
		}
		if(t->mm.dseg == base &&  c&DS_SWAP)
		{
			t->mm.cseg = o->page_base;
			t->mm.flags &= ~DS_SWAP;
		}
		/* If we are swapped in and we are running then we are
		   now runnable on the task queue */
		if(c && !t->mm.flags && t->state == TASK_RUNNING)
			add_to_runqueue(t);
	}
	/* Our equivalent of the Linux swap cache. Try and avoid writing CS
	   back. Need to kill segments on last exit for this to work, and
	   keep a table - TODO
	if(chint==0) */
	{
		so->refcount=0;
		so->flags = HOLE_FREE;
		sweep_holes(&swapmap);
	}
	return 0;
}

/*
 *	When the swapper has found a task it wishes to make run we do the dirty
 *	work. On failure we return -1
 */
static int make_runnable(t)
register struct task_struct *t;
{
	if(t->mm.flags&CS_SWAP)
		if(swap_in(t->mm.cseg, 1)==-1)
			return -1;
	if(t->mm.flags&DS_SWAP)
		return swap_in(t->mm.dseg,0);
	return 0;
}

#if 0
/*
 *	Do an iteration of the swapper
 */
 
static int do_swapper_run()
{
	while(make_runnable(swap_target)==-1)
	{
		seg_t t=swap_strategy();
		if(swap_out(t)==-1)
			return -1;	/* Kill swap_target ?? */
	}
	return 0;
}
#endif
