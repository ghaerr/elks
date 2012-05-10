/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Utility routines.
 */

#include "../sash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>

#define BUF_SIZE 1024 

typedef	struct	chunk	CHUNK;
#define	CHUNKINITSIZE	4
struct	chunk	{
	CHUNK	*next;
	char	data[CHUNKINITSIZE];	/* actually of varying length */
};


static	CHUNK *	chunklist;


/*
 * Allocate a chunk of memory (like malloc).
 * The difference, though, is that the memory allocated is put on a
 * list of chunks which can be freed all at one time.  You CAN NOT free
 * an individual chunk.
 */
char *
getchunk(size)
{
	CHUNK	*chunk;

	if (size < CHUNKINITSIZE)
		size = CHUNKINITSIZE;

	chunk = (CHUNK *) malloc(size + sizeof(CHUNK) - CHUNKINITSIZE);
	if (chunk == NULL)
		return NULL;

	chunk->next = chunklist;
	chunklist = chunk;

	return chunk->data;
}


/*
 * Free all chunks of memory that had been allocated since the last
 * call to this routine.
 */
void
freechunks()
{
	CHUNK	*chunk;

	while (chunklist) {
		chunk = chunklist;
		chunklist = chunk->next;
		free((char *) chunk);
	}
}
