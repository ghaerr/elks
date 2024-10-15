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
 * hashtable
 */

typedef struct hashtable T_HashTable;
typedef void *T_HashTableEl;
typedef size_t (*T_HashFunc)(void *);
typedef int (*T_ComparFunc)(void *, void *);


int make_ht(T_HashFunc f1, T_HashFunc f2, T_ComparFunc c, size_t size,
	    T_HashTable **H);
int hash_add(T_HashTable *H, T_HashTableEl *E, size_t *hint);
int hash_remove(T_HashTable *H, T_HashTableEl *E, size_t hint);
int hash_lookup(T_HashTable *H, T_HashTableEl *E, T_HashTableEl **E2,
		size_t *hint);
int free_ht(T_HashTable *H, T_HashFunc entry_free);

