
#include "list.h"


void list_init (list_root_t * root)
	{
	list_node_t * node = &root->node;
	node->prev = node;
	node->next = node;
	root->count = 0;
	}


#define LIST_LINK \
	prev->next = node; \
	node->prev = prev; \
	next->prev = node; \
	node->next = next; \
	/**/


static void insert_before (list_node_t * next, list_node_t * node)
	{
	list_node_t * prev = next->prev;
	LIST_LINK
	}

static void insert_after (list_node_t * prev, list_node_t * node)
	{
	list_node_t * next = prev->next;
	LIST_LINK
	}


void list_add_tail (list_root_t * root, list_node_t * node)
	{
	insert_before (&root->node, node);
	root->count++;
	}

void list_add_head (list_root_t * root, list_node_t * node)
	{
	insert_after (&root->node, node);
	root->count++;
	}
