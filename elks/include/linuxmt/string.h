#ifndef LX86_LINUXMT_STRING_H
#define LX86_LINUXMT_STRING_H

extern char *strcpy();
extern char *strncpy();
extern char *strcat();
extern char *strncat();
extern int strcmp();
extern int strncmp();
extern char *strchr();
extern size_t strlen();
extern size_t strnlen();
extern size_t strspn();
extern char *strpbrk();
extern char *strtok();
extern char *memset();
extern char *bcopy();
extern char *memcpy();
extern char *memmove();
extern int memcmp();
extern char *memscan();
extern char *strstr();

/*
 * Include machine specific routines
 */
#include <arch/string.h>

#endif
