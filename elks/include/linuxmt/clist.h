#ifndef __LINUX_CLIST_H
#define __LINUX_CLIST_H
/*
 *	Character device list handlers.
 */
 
#define CL_NCHAR	12

struct clist
{
	struct clist *cl_next;
	unsigned char cl_head;
	unsigned char cl_tail;
	unsigned char cl_data[CL_NCHAR];
};

struct clhead
{
	struct clist *ch_head;
	struct clist *ch_tail;
	int ch_count;
	int ch_limit;
};


#define NUM_CLIST	64

extern struct clist *cl_alloc();
extern void cl_free();
extern int cl_addch();
extern int cl_getch();
extern void cl_empty();
extern int cl_delch();
extern void cl_init();
#endif
