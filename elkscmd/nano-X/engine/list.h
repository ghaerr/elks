#ifndef __LIST_H
#define __LIST_H
/*
 * link list library
 *
 * 1/28/98 g haerr
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 */

/* dbl linked list data structure*/
typedef struct _list {			/* LIST must be first decl in struct*/
	struct _list *	next;		/* next item*/
	struct _list *	prev;		/* previous item*/
} LIST, *PLIST;

/* dbl linked list head data structure*/
typedef struct _listhead {
	struct _list *	head;		/* first item*/
	struct _list *	tail;		/* last item*/
} LISTHEAD, *PLISTHEAD;

/* list functions*/
void * 	ItemAlloc(unsigned int size);
void	ListAdd(PLISTHEAD pHead,PLIST pItem);
void	ListInsert(PLISTHEAD pHead,PLIST pItem);
void	ListRemove(PLISTHEAD pHead,PLIST pItem);
#define ItemNew(type)	((type *)ItemAlloc(sizeof(type)))
#define ItemFree(ptr)	free((void *)ptr)

/* field offset*/
#define ITEM_OFFSET(type, field)    ((long)&(((type *)0)->field))

/* return base item address from list ptr*/
#define ItemAddr(p,type,list)	((type *)((long)p - ITEM_OFFSET(type,list)))

#endif /* __LIST_H*/
