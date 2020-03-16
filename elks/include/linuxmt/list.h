// Kernel library
// Double-linked list

#pragma once

struct list {
	struct list * prev;
	struct list * next;
};

typedef struct list list_s;

void list_init (list_s * root);

void list_insert_before (list_s * next, list_s * node);
void list_insert_after  (list_s * prev, list_s * node);

void list_remove (list_s * node);
