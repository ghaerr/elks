// Kernel library
// Local heap management

#pragma once

#include <linuxmt/types.h>

void * heap_alloc (word_t size);
void heap_free (void * data);

void heap_add (void * data, word_t size);

