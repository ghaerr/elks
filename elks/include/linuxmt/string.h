#ifndef LX86_LINUXMT_STRING_H
#define LX86_LINUXMT_STRING_H

#include <linuxmt/types.h>

/* The following prototypes all match the current GNU manpage prototypes.
 * It is still to be confirmed whether these also match the declarations
 * and the usage made thereof.
 */

extern char *strcpy(char *,char *);
extern char *strncpy(char *,char *,size_t);
extern char *strcat(char *,char *);
extern char *strncat(char *,char *,size_t);
extern int strcmp(char *,char *);
extern int strncmp(char *,char *,size_t);
extern char *strchr(char *,int);
extern size_t strlen(char *);
extern size_t strnlen(char *,size_t);
extern size_t strspn(char *,char *);
extern size_t strcspn(char *,char *);
extern char *strpbrk(char *,char *);
extern char *strtok(char *,char *);
extern void bcopy(void *,void *,int);
extern void *memset(void *,int,size_t);
extern void *memcpy(void *,void *,size_t);
extern void *memmove(void *,void *,size_t);
extern int memcmp(void *,void *,size_t);
extern char *strstr(char *,char *);

/* The following prototype does not have a GNU manpage, so matches the
 * usage in the ELKS kernel source.
 */

extern char *memscan(char *,int,size_t);

/*
 * Include machine specific routines
 */
#include <arch/string.h>

#endif
