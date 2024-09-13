#ifndef __LINUXMT_STRING_H
#define __LINUXMT_STRING_H

#include <linuxmt/types.h>

/* The following prototypes all match the current GNU manpage prototypes.
 * It is still to be confirmed whether these also match the declarations
 * and the usage made thereof.
 */

extern char *strcpy(char *,const char *);
extern char *strncpy(char *,const char *,size_t);
extern char *strcat(char *,const char *);
extern char *strncat(char *,const char *,size_t);
extern int strcmp(const char *,const char *);
extern int strncmp(const char *,const char *,size_t);
extern char *strchr(const char *,int);
extern size_t strlen(const char *);
extern size_t strnlen(const char *,size_t);
extern size_t strspn(char *,char *);
extern size_t strcspn(char *,char *);
extern char *strpbrk(char *,char *);
extern char *strtok(char *,char *);
extern void bcopy(void *,void *,int);
extern void *memset(void *,int,size_t);
extern void *memcpy(void *,const void *,size_t);
extern void *memmove(void *,const void *,size_t);
extern int memcmp(const void *,const void *,size_t);
extern char *strstr(char *,char *);

/* The following prototype does not have a GNU manpage, so matches the
 * usage in the ELKS kernel source.
 */

extern void *memscan(void *,int,size_t);
extern long simple_strtol(const char *,int);
extern int atoi(const char *);

/*
 * Include machine specific routines
 */
#include <arch/string.h>

#endif
