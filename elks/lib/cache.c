// Kernel library
// Generic cache

#include <linuxmt/heap.h>
#include <linuxmt/cache.h>
#include <linuxmt/wait.h>
#include <linuxmt/string.h>

/////////////////////////////////////////////////////////////////////////////////
// Typical usage on a very simple string cache

struct string_entry_s {
	cache_entry_t cache;
	char * str;
};

typedef struct string_entry_s string_entry_t;

static cache_entry_t * string_alloc ()
{
	string_entry_t * se = heap_alloc (sizeof (string_entry_t));
	if (!se) return 0;
	return &se->cache;
}

static void string_free (cache_entry_t * e)
{
	string_entry_t * se = structof (e, string_entry_t, cache);
	if (se->str) {
		heap_free (se->str);
		se->str = 0;
	}
	heap_free (se);
}

static int string_test (cache_entry_t * e, void * key)
{
	string_entry_t * se = structof (e, string_entry_t, cache);
	return strcmp (se->str, (char *) key);
}

static int string_load (cache_entry_t * e, void * key)
{
	string_entry_t * se = structof (e, string_entry_t, cache);
	char * str = heap_alloc (strlen ((char *) key));
	if (!str) return 1;
	se->str = str;
	cache_valid (e);
	return 0;
}

static void string_store (cache_entry_t * e)
{
	// nothing to store
	cache_clean (e);
}

static cache_type_t string_cache = {
	.alloc = string_alloc,
	.free = string_free,
	.test = string_test,
	.load = string_load,
	.store = string_store,
	.max = 1
};

int cache_test_main ()
{
	// Single entry cache

	cache_head_init ();
	cache_type_init (&string_cache);

	// First entry will be kept

	cache_entry_t * e = cache_find (&string_cache, "first");
	if (!e || !cache_lock (e)) return 1;
	//string_entry_t * se = structof (e, string_entry_t, cache);
	//kprintf ("entry=%s\n", se->str);
	cache_clean (e);
	cache_put (e);

	// Second entry will be kept
	// and will force first entry deletion
	// as maximum is one

	e = cache_find (&string_cache, "second");
	if (!e || !cache_lock (e)) return 1;
	//se = structof (e, string_entry_t, cache);
	//kprintf ("entry=%s\n", se->str);
	cache_clean (e);
	cache_put (e);

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////

// Global cache head

static cache_head_t _head;

// Initialize cache head

void cache_head_init ()
	{
	list_init (&_head.root);
	_head.lock = 0;
	}

// Initialize cache type

void cache_type_init (cache_type_t * t)
{
	list_init (&t->root);
	t->lock = 0;
	t->now = 0;
}

// Delete one entry

static void cache_delete (cache_type_t * t, cache_entry_t * e)
{
	// Backup dirty data

	if (e->valid && e->dirty && t->store)
		(t->store) (e);

	// Remove from global list

	lock_wait (&_head.lock);
	list_remove (&e->hnode);
	unlock_event (&_head.lock);

	// Remove from type list

	list_remove (&e->tnode);
	t->now--;

	// Free entry memory

	(t->free) (e);
}

static word_t cache_head_empty (word_t max)
	{
	// TODO: complete the function
	return 0;
	}

// Delete some entries of same type

static word_t cache_type_empty (cache_type_t * t, word_t max)
{
	word_t count = 0;

	list_t * now = &t->root;
	list_t * prev = now->prev;

	while (t->now && count < max) {

		// Start from LRU tail

		now = prev;
		prev = now->prev;

		cache_entry_t * e = structof (now, cache_entry_t, tnode);
		if (atomic_get (&e->ref) == 1) {

			// Entry not referenced out of LRU
			// So no need to lock it

			cache_delete (t, e);
			count++;
		}
	}

	return count;
}

// Allocate cache entry

static cache_entry_t * cache_alloc (cache_type_t * t)
{
	cache_entry_t * e = 0;

	while (1) {

		// Check maximum entries count

		if (t->max && t->now >= t->max) {

			// Cache full
			// Try to delete one entry

			word_t count = cache_type_empty (t, 1);
			if (!count) break;
		}

		while (1) {

			// Allocate new entry from heap

			e = (t->alloc) (t);
			if (!e) break;

			// No memory left
			// Conflict with another memory consumer
			// Try to delete globally one entry and retry

			word_t count = cache_head_empty (1);
			if (!count) break;
		}

		break;
	}

	return e;
}

// Create a new cache entry and load data

static cache_entry_t * cache_new (cache_type_t * t, void * key)
{
	cache_entry_t * e;

	while (1) {
		e = cache_alloc (t);
		if (!e) break;

		e->valid = 0;  // not loaded yet
		e->lock = 1;   // for data loading
		e->ref = 2;    // list and returned references

		e->type = t;
		list_insert_after (&t->root, &e->tnode);  // add head

		// Add to global list

		lock_wait (&_head.lock);
		list_insert_after (&_head.root, &e->hnode);  // add head
		unlock_event (&_head.lock);

		// Anyone can reference the entry now
		// but data is still locked until loaded

		unlock_event (&t->lock);

		// User function shall validate the entry
		// after successful data load

		int err = (t->load) (e, key);
		if (!err) break;

		// Failed to load data
		// so leave the entry as invalid

		unlock_event (&e->lock);
		break;
	}

	return e;
}

// Find cache entry from key

static cache_entry_t * cache_search (cache_type_t * t, void * key)
{
	list_t * n = &t->root;
	while (1)
		{
		n = n->next;
		if (n == &t->root) break;

		cache_entry_t * e = structof (n, cache_entry_t, tnode);
		if (!(t->test) (e, key)) return e;
		}

	return 0;
}

// Find cache entry (existing or new one)

cache_entry_t * cache_find (cache_type_t * t, void * key)
{
	cache_entry_t * e;

	while (1) {

		// Search entry

		lock_wait (&t->lock);

		e = cache_search (t, key);
		if (e) {

			// Add returned reference and unlock

			atomic_inc (&e->ref);
			unlock_event (&t->lock);
			break;
		}

		// New entry

		e = cache_new (t, key);
		break;
		}

	return e;
}

// Reference counting

cache_entry_t * cache_get (cache_entry_t * e)
{
	atomic_inc (&e->ref);
	return e;
}

void cache_put (cache_entry_t * e)
{
	cache_type_t * t = e->type;
	lock_wait (&t->lock);
	atomic_dec (&e->ref);
	if (atomic_get (&e->ref) == 1 && !t->max)
		cache_delete (t, e);
	else {

		// Update the LRU lists

		list_remove (&e->tnode);
		list_insert_after (&t->root, &e->tnode);  // add head

		lock_wait (&_head.lock);
		list_remove (&e->hnode);
		list_insert_after (&_head.root, &e->hnode);  // add head
		unlock_event (&_head.lock);
	}

	unlock_event (&t->lock);
}

// Begin data access

bool_t cache_lock (cache_entry_t * e)
{
	lock_wait (&e->lock);
	return e->valid;
}

// End data access

void cache_valid (cache_entry_t * e)
{
	e->valid = 1;
	e->dirty = 0;
	unlock_event (&e->lock);
}

void cache_clean (cache_entry_t * e)
{
	e->dirty = 0;
	unlock_event (&e->lock);
}

void cache_dirty (cache_entry_t * e)
{
	e->dirty = 1;
	unlock_event (&e->lock);
}
