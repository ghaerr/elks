// Kernel library
// Double linked list

#include <linuxmt/list.h>

void list_init (list_s * root)
	{
	root->prev = root;
	root->next = root;
	}

#define LIST_LINK \
	prev->next = node; \
	node->prev = prev; \
	next->prev = node; \
	node->next = next; \
	/**/

void list_insert_before (list_s * next, list_s * node)
{
	list_s * prev = next->prev;
	LIST_LINK
}

void list_insert_after (list_s * prev, list_s * node)
{
	list_s * next = prev->next;
	LIST_LINK
}

void list_remove (list_s * node)
{
	list_s * prev = node->prev;
	list_s * next = node->next;
	prev->next = next;
	next->prev = prev;
}
