/* List of libc functions implemented in assembly */

#pragma once

#ifdef __ia16__
#define LIBC_ASM_MEMCPY
#define LIBC_ASM_MEMSET
#define LIBC_ASM_STRCPY
#define LIBC_ASM_STRLEN
#endif
