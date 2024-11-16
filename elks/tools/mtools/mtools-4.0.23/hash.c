/*  Copyright 1996,1997,2001,2002,2009 Alain Knaff.
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
 *
 * hash.c - hash table.
 */

#include "sysincludes.h"
#include "htable.h"
#include "mtools.h"

struct hashtable {
  T_HashFunc f1,f2;
  T_ComparFunc compar;
  size_t size;  /* actual size of the array */
  size_t fill;  /* number of deleted or in use slots */
  size_t inuse; /* number of slots in use */
  size_t max;   /* maximal number of elements to keep efficient */
  T_HashTableEl *entries;
};

static size_t sizes[]=
	{ 5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853,
	  25717, 51437, 102877, 205759, 411527, 823117, 1646237,
	  3292489, 6584983, 13169977, 26339969, 52679969, 105359939,
	  210719881, 421439783, 842879579, 1685759167, 0 };
static int deleted=0;
static int unallocated=0;

static int alloc_ht(T_HashTable *H, size_t size)
{
  int i;
  size_t ii;

  for(i=0; sizes[i]; i++)
    if (sizes[i] > size*4 )
      break;
  if (!sizes[i])
    for(i=0; sizes[i]; i++)
      if (sizes[i] > size*2 )
	break;
  if (!sizes[i])
    for(i=0; sizes[i]; i++)
      if (sizes[i] > size)
	break;
  if(!sizes[i])
    return -1;
  size = sizes[i];
  if(size < H->size)
	  size = H->size; /* never shrink the table */
  H->max = size * 4 / 5 - 2;
  H->size = size;
  H->fill = 0;
  H->inuse = 0;
  H->entries = NewArray(size, T_HashTableEl);
  if (H->entries == NULL)
    return -1; /* out of memory error */
  
  for(ii=0; ii < size; ii++)
    H->entries[ii] = &unallocated;
  return 0;
}

int make_ht(T_HashFunc f1, T_HashFunc f2, T_ComparFunc c, size_t size,
	    T_HashTable **H)
{
  *H = New(T_HashTable);
  if (*H == NULL){
    return -1; /* out of memory error */
  }
  
  (*H)->f1 = f1;
  (*H)->f2 = f2;
  (*H)->compar = c;
  (*H)->size = 0;
  if(alloc_ht(*H,size))
    return -1;
  return 0;
}

int free_ht(T_HashTable *H, T_HashFunc entry_free)
{
  size_t i;
  if(entry_free)
    for(i=0; i< H->size; i++)
      if (H->entries[i] != &unallocated &&
	  H->entries[i] != &deleted)
	entry_free(H->entries[i]);
  Free(H->entries);
  Free(H);
  return 0;
}

/* add into hash table without checking for repeats */
static int _hash_add(T_HashTable *H,T_HashTableEl *E, size_t *hint)
{
  size_t f2, pos;
  int ctr;

  pos = H->f1(E) % H->size;
  f2 = (size_t) -1;
  ctr = 0;
  while(H->entries[pos] != &unallocated &&
	H->entries[pos] != &deleted){
    if (f2 == (size_t) -1)
      f2 = H->f2(E) % (H->size - 1);
    pos = (pos+f2+1) % H->size;
    ctr++;
  }
  if(H->entries[pos] == &unallocated)
     H->fill++; /* only increase fill if the previous element was not yet
		 * counted, i.e. unallocated */
  H->inuse++;
  H->entries[pos] = E;
  if(hint)
	  *hint = pos;
  return 0;
}

static int rehash(T_HashTable *H)
{
  size_t size,i;
  T_HashTableEl *oldentries;
  /* resize the table */
  
  size = H->size;
  oldentries = H->entries;
  if(alloc_ht(H,((H->inuse+1)*4+H->fill)/5))
	  return -1;

  for(i=0; i < size; i++){
    if(oldentries[i] != &unallocated && oldentries[i] != &deleted)
      _hash_add(H, oldentries[i], 0);
  }
  Free(oldentries);
  return 0;
}

int hash_add(T_HashTable *H, T_HashTableEl *E, size_t *hint)
{
  if (H->fill >= H->max)
    rehash(H);
  if (H->fill == H->size)
    return -1; /*out of memory error */
  return _hash_add(H,E, hint);
}


/* add into hash table without checking for repeats */
static int _hash_lookup(T_HashTable *H,T_HashTableEl *E, T_HashTableEl **E2,
			size_t *hint, int isIdentity)
{
	size_t f2, pos, upos, ttl;

  pos = H->f1(E) % H->size;
  ttl = H->size;
  f2 = (size_t) -1;
  upos = (size_t) -1;
  while(ttl &&
	H->entries[pos] != &unallocated &&
	(H->entries[pos] == &deleted ||
	 ((isIdentity || H->compar(H->entries[pos], E) != 0) &&
	  (!isIdentity || H->entries[pos] != E)))){
    if (f2 == (size_t) -1)
      f2 = H->f2(E) % (H->size - 1);
    if (upos == (size_t) -1 && H->entries[pos] == &deleted)
      upos = pos;
    pos = (pos+f2+1) % H->size;
    ttl--;
  }
  if(H->entries[pos] == &unallocated || !ttl)
    return -1;
  if (upos != (size_t) -1){
    H->entries[upos] = H->entries[pos];
    H->entries[pos] = &deleted;
    pos = upos;
  }
  if(hint)
    *hint = pos;
  *E2= H->entries[pos];
  return 0;
}


int hash_lookup(T_HashTable *H,T_HashTableEl *E, T_HashTableEl **E2,
		size_t *hint)
{
	return _hash_lookup(H, E, E2, hint, 0);
}

/* add into hash table without checking for repeats */
int hash_remove(T_HashTable *H,T_HashTableEl *E, size_t hint)
{
  T_HashTableEl *E2;

  if (hint >=0 && hint < H->size &&
      H->entries[hint] == E){
    H->inuse--;
    H->entries[hint] = &deleted;
    return 0;
  }

  if(_hash_lookup(H, E, &E2, &hint, 1)) {
	  fprintf(stderr, "Removing non-existent entry\n");
	  exit(1);
  }
  H->inuse--;
  H->entries[hint] = &deleted;
  return 0;
}
