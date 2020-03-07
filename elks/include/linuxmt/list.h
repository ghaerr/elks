// Kernel library
// Double-linked list

#pragma once

#include <stddef.h>

#define structof(p,t,m) ((t *) ((char *) (p) - offsetof (t,m)))

struct list_s {
	struct list_s * prev;
	struct list_s * next;
};

typedef struct list_s list_t;

void list_init (list_t * root);

void list_insert_before (list_t * next, list_t * node);
void list_insert_after  (list_t * prev, list_t * node);

void list_remove (list_t * node);
