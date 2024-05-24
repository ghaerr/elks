#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <features.h>
#include <stddef.h>
#include __SYSINC__(types.h)

#ifdef __WATCOMC__
#include <stdint.h>
#endif

#ifdef __GNUC__
/* ia16-elf-gcc only supports small and medium models */
typedef int         intptr_t;
#endif

typedef intptr_t    ssize_t;

#endif
