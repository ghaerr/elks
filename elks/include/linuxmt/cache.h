// Kernel library
// Generic cache

#pragma once

#include <linuxmt/list.h>
#include <linuxmt/lock.h>

// Forward declarations

struct cache_type_s;

// Cached entry

struct cache_entry_s {
	struct cache_type_s * type;

	list_t tnode;  // type LRU list
	list_t hnode;  // head LRU list
	atomic_t ref;  // reference count
	lock_t lock;   // data lock
	bool_t valid;  // valid flag
	bool_t dirty;  // dirty flag
};

typedef struct cache_entry_s cache_entry_t;

// Cache type

struct cache_type_s {
	list_t root;  // type LRU list
	lock_t lock;  // access mutex

	word_t max;   // maximum size (0: no retention)
	word_t now;   // current size

	cache_entry_t * (* alloc) ();
	void (* free) (cache_entry_t *);

	int (* test) (cache_entry_t *, void * key);  // test entry key
	int (* load) (cache_entry_t *, void * key);  // read data from backend
	void (* store) (cache_entry_t *);            // store data to backend
};

typedef struct cache_type_s cache_type_t;

// Cache head

struct cache_head_s {
	list_t root;  // global LRU list
	lock_t lock;  // access mutex
};

typedef struct cache_head_s cache_head_t;

extern cache_head_t _cache_head;

// Cache functions

void cache_head_init ();
void cache_type_init (cache_type_t *);

cache_entry_t * cache_find (cache_type_t *, void * key);

// Reference counting

cache_entry_t * cache_get (cache_entry_t *);
void cache_put (cache_entry_t *);

// Data access functions

bool_t cache_lock (cache_entry_t *);

void cache_valid (cache_entry_t *);
void cache_clean (cache_entry_t *);
void cache_dirty (cache_entry_t *);
