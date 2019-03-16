// Machine dependent types

#pragma once

// For GCC stddef.h

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned int
#endif

// Optimal boolean size for IA16 is word_t

typedef unsigned short bool_t;
