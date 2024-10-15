/*  Copyright 1998,2001-2003,2007-2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "sysincludes.h"
#include "vfat.h"
#include "dirCache.h"
#include "dirCacheP.h"
#include <assert.h>


#define BITS_PER_INT (sizeof(unsigned int) * 8)


static __inline__ uint32_t rol(uint32_t arg, int shift)
{
	arg &= 0xffffffff; /* for 64 bit machines */
	return (arg << shift) | (arg >> (32 - shift));
}


static uint32_t calcHash(wchar_t *name)
{
	uint32_t hash;
	unsigned int i;
	wint_t c;

	hash = 0;
	i = 0;
	while(*name) {
		/* rotate it */
		hash = rol(hash,5); /* a shift of 5 makes sure we spread quickly
				     * over the whole width, moreover, 5 is
				     * prime with 32, which makes sure that
				     * successive letters cannot cover each
				     * other easily */
		c = towupper((wint_t)*name);		
		hash ^=  (c * (c+2)) ^ (i * (i+2));
		hash &= 0xffffffff;
		i++, name++;
	}
	hash = hash * (hash + 2);
	/* the following two xors make sure all info is spread evenly over all
	 * bytes. Important if we only keep the low order bits later on */
	hash ^= (hash & 0xfff) << 12;
	hash ^= (hash & 0xff000) << 24;
	return hash;
}

static unsigned int addBit(unsigned int *bitmap,
			   unsigned int hash, int checkOnly)
{
	unsigned int bit;
	int entry;

	bit = 1u << (hash % BITS_PER_INT);
	entry = (hash / BITS_PER_INT) % DC_BITMAP_SIZE;
	
	if(checkOnly)
		return bitmap[entry] & bit;
	else {
		bitmap[entry] |= bit;
		return 1;
	}
}

static int _addHash(dirCache_t *cache, unsigned int hash, int checkOnly)
{
	return
		addBit(cache->bm0, hash, checkOnly) &&
		addBit(cache->bm1, rol(hash,12), checkOnly) &&
		addBit(cache->bm2, rol(hash,24), checkOnly);
}


static void addNameToHash(dirCache_t *cache, wchar_t *name)
{	
	_addHash(cache, calcHash(name), 0);
}

static void hashDce(dirCache_t *cache, dirCacheEntry_t *dce)
{
	if(dce->beginSlot != cache->nrHashed)
		return;
	cache->nrHashed = dce->endSlot;
	if(dce->longName)
		addNameToHash(cache, dce->longName);
	addNameToHash(cache, dce->shortName);
}

int isHashed(dirCache_t *cache, wchar_t *name)
{
	int ret;

	ret =  _addHash(cache, calcHash(name), 1);
	return ret;
}

int growDirCache(dirCache_t *cache, unsigned int slot)
{
	if((int) slot < 0) {
		fprintf(stderr, "Bad slot %d\n", slot);
		exit(1);
	}

	if( cache->nr_entries <= slot) {
		unsigned int i;
		
		cache->entries = realloc(cache->entries,
					 (slot+1) * 2 *
					 sizeof(dirCacheEntry_t *));
		if(!cache->entries)
			return -1;
		for(i= cache->nr_entries; i < (slot+1) * 2; i++) {
			cache->entries[i] = 0;
		}
		cache->nr_entries = (slot+1) * 2;
	}
	return 0;
}

dirCache_t *allocDirCache(Stream_t *Stream, unsigned int slot)
{
	dirCache_t **dcp;

	if((int)slot < 0) {
		fprintf(stderr, "Bad slot %d\n", slot);
		exit(1);
	}

	dcp = getDirCacheP(Stream);
	if(!*dcp) {
		*dcp = New(dirCache_t);
		if(!*dcp)
			return 0;
		(*dcp)->entries = NewArray((slot+1)*2+5, dirCacheEntry_t *);
		if(!(*dcp)->entries) {
			free(*dcp);
			return 0;
		}
		(*dcp)->nr_entries = (slot+1) * 2;
		memset( (*dcp)->bm0, 0, sizeof((*dcp)->bm0));
		memset( (*dcp)->bm1, 0, sizeof((*dcp)->bm1));
		memset( (*dcp)->bm2, 0, sizeof((*dcp)->bm1));
		(*dcp)->nrHashed = 0;
	} else
		if(growDirCache(*dcp, slot) < 0)
			return 0;
	return *dcp;
}

/* Free a range of entries. The range being cleared is always aligned
 * on the begin of an entry. Entries which are cleared entirely are
 * free'd. Entries which are cleared half-way (can only happen at end)
 * are "shortened" by moving its beginSlot
 * If this was a range representing space after end-mark, return beginning
 * of range if it had been shortened in its lifetime (which means that end
 * mark must have moved => may need to be rewritten)
 */
static int freeDirCacheRange(dirCache_t *cache,
			     unsigned int beginSlot,
			     unsigned int endSlot)
{
	dirCacheEntry_t *entry;
	unsigned int clearBegin;
	unsigned int clearEnd;
	unsigned int i;

	if(endSlot < beginSlot) {
		fprintf(stderr, "Bad slots %d %d in free range\n",
			beginSlot, endSlot);
		exit(1);
	}

	while(beginSlot < endSlot) {
		entry = cache->entries[beginSlot];
		if(!entry) {
			beginSlot++;
			continue;
		}
		
		/* Due to the way this is called, we _always_ de-allocate
		 * starting from beginning... */
		assert(entry->beginSlot == beginSlot);

		clearEnd = entry->endSlot;
		if(clearEnd > endSlot)
			clearEnd = endSlot;
		clearBegin = beginSlot;
		
		for(i = clearBegin; i <clearEnd; i++)
			cache->entries[i] = 0;

		entry->beginSlot = clearEnd;

		if(entry->beginSlot == entry->endSlot) {
			int needWriteEnd = 0;
			if(entry->endMarkPos != -1 &&
			   entry->endMarkPos < (int) beginSlot)
				needWriteEnd = 1;

			if(entry->longName)
				free(entry->longName);
			if(entry->shortName)
				free(entry->shortName);
			free(entry);
			if(needWriteEnd) {
				return (int) beginSlot;
			}
		}

		beginSlot = clearEnd;
	}
	return -1;
}

static dirCacheEntry_t *allocDirCacheEntry(dirCache_t *cache,
					   unsigned int beginSlot,
					   unsigned int endSlot,
					   dirCacheEntryType_t type)
{
	dirCacheEntry_t *entry;
	unsigned int i;

	if(growDirCache(cache, endSlot) < 0)
		return 0;

	entry = New(dirCacheEntry_t);
	if(!entry)
		return 0;
	entry->type = type;
	entry->longName = 0;
	entry->shortName = 0;
	entry->beginSlot = beginSlot;
	entry->endSlot = endSlot;
	entry->endMarkPos = -1;

	freeDirCacheRange(cache, beginSlot, endSlot);
	for(i=beginSlot; i<endSlot; i++) {
		cache->entries[i] = entry;
	}
	return entry;
}

dirCacheEntry_t *addUsedEntry(dirCache_t *cache,
			      unsigned int beginSlot,
			      unsigned int endSlot,
			      wchar_t *longName, wchar_t *shortName,
			      struct directory *dir)
{
	dirCacheEntry_t *entry;

	if(endSlot < beginSlot) {
		fprintf(stderr,
			"Bad slots %d %d in add used entry\n",
			beginSlot, endSlot);
		exit(1);
	}


	entry = allocDirCacheEntry(cache, beginSlot, endSlot, DCET_USED);
	if(!entry)
		return 0;
	
	entry->beginSlot = beginSlot;
	entry->endSlot = endSlot;
	if(longName)
		entry->longName = wcsdup(longName);
	entry->shortName = wcsdup(shortName);
	entry->dir = *dir;
	hashDce(cache, entry);
	return entry;
}

/* Try to merge free slot at position "slot" with slot at previous position
 * After successful merge, both will point to a same DCE (the one which used
 * to be previous)
 * Only does something if both of these slots are actually free
 */
static void mergeFreeSlots(dirCache_t *cache, unsigned int slot)
{
	dirCacheEntry_t *previous, *next;
	unsigned int i;

	if(slot == 0)
		return;
	previous = cache->entries[slot-1];
	next = cache->entries[slot];
	if(next && next->type == DCET_FREE &&
	   previous && previous->type == DCET_FREE) {
		for(i=next->beginSlot; i < next->endSlot; i++)
			cache->entries[i] = previous;
		previous->endSlot = next->endSlot;
		previous->endMarkPos = next->endMarkPos;
		free(next);		
	}
}

/* Create a free range extending from beginSlot to endSlot, and try to merge
 * it with any neighboring free ranges */
dirCacheEntry_t *addFreeEndEntry(dirCache_t *cache,
				 unsigned int beginSlot,
				 unsigned int endSlot,
				 int isAtEnd)
{
	dirCacheEntry_t *entry;

	if(beginSlot < cache->nrHashed)
		cache->nrHashed = beginSlot;

	if(endSlot < beginSlot) {
		fprintf(stderr, "Bad slots %d %d in add free entry\n",
			beginSlot, endSlot);
		exit(1);
	}

	if(endSlot == beginSlot)
		return 0;
	entry = allocDirCacheEntry(cache, beginSlot, endSlot, DCET_FREE);
	if(isAtEnd)
		entry->endMarkPos = (int) beginSlot;
	mergeFreeSlots(cache, beginSlot);
	mergeFreeSlots(cache, endSlot);
	return cache->entries[beginSlot];
}

dirCacheEntry_t *addFreeEntry(dirCache_t *cache,
			      unsigned int beginSlot,
			      unsigned int endSlot)
{
	return addFreeEndEntry(cache, beginSlot, endSlot, 0);
}

dirCacheEntry_t *addEndEntry(dirCache_t *cache, unsigned int pos)
{
	return allocDirCacheEntry(cache, pos, pos+1u, DCET_END);
}

dirCacheEntry_t *lookupInDircache(dirCache_t *cache, unsigned int pos)
{
	if(growDirCache(cache, pos+1) < 0)
		return 0;
	return cache->entries[pos];	
}

void freeDirCache(Stream_t *Stream)
{
	dirCache_t *cache, **dcp;

	dcp = getDirCacheP(Stream);
	cache = *dcp;
	if(cache) {
		int n;
		n=freeDirCacheRange(cache, 0, cache->nr_entries);
		if(n >= 0)
			low_level_dir_write_end(Stream, n);
		free(cache);
		*dcp = 0;
	}
}
