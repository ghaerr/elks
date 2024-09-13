#ifndef __STRING_H
#define __STRING_H

#include <sys/types.h>

/* Basic string functions */

size_t strlen(const char * s);
char *strcpy(char*, const char*);
char *strcat(char * dest, const char * src);
int strcmp(const char *s1, const char *s2);

char * strncpy(char*, const char*, size_t);
char * strncat(char*, const char*, size_t);
int strncmp(const char * s1, const char * s2, size_t n);

char * strstr(const char *, const char *);
char * strchr(const char * s, int c);
char * strrchr(const char * s, int c);

char * strdup(const char*);

/* Basic mem functions */
void * memcpy(void * dest, const void * src, size_t n);
void * memccpy(void*, const void*, int, size_t);
void * memchr(const void*, const int, size_t);
void * memset(void*, int, size_t);
int memcmp(const void*, const void*, size_t);
void * memmove(void*, const void*, size_t);

void __far *fmemset(void __far *buf, int c, size_t l);

/* Error messages */
char * strerror(int);

/* Minimal (very!) locale support */
#define strcoll strcmp
#define strxfrm strncpy

/* deprecated BSDisms */
char * index(const char * s, int c);  /* replace by strchr */
char * rindex(const char * s, int c); /* replace by strrchr */

/* Other common BSD functions */
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
char *strtok(char * str, const char * delim);
char *strpbrk(const char *, const char *);
char *strsep(char **, const char *);

size_t strcspn(const char *, const char *);
size_t strspn(const char *, const char *);

/* Linux silly hour */
char *strfry(char *);

void bzero(void * s, size_t n);

/* TODO: this is removed in POSIX-1.2008
 * TODO: replace by memcpy or memmove
 */
#define bcopy(s, d, n) memcpy((d), (s), (n))

#endif
