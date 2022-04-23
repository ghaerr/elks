
#ifndef __STRING_H
#define __STRING_H

#pragma once

#include <features.h>
#include <stddef.h>

/* Basic string functions */

extern char * strcpy __P ((char*, __const char*));
int strcmp(const char *s1, const char *s2);

extern char * strncat __P ((char*, char*, size_t));
extern char * strncpy __P ((char*, char*, size_t));

char * strchr  (const char * s, int c);
char * strrchr (const char * s, int c);

extern char * strdup __P ((const char*));

/* Basic mem functions */
extern void * memccpy __P ((void*, void*, int, size_t));
extern void * memchr __P ((__const void*, __const int, size_t));
extern void * memset __P ((void*, int, size_t));
extern int memcmp __P ((__const void*, __const void*, size_t));

extern void * memmove __P ((void*, void*, size_t));

/* Error messages */
extern char * strerror __P ((int));

/* Minimal (very!) locale support */
#define strcoll strcmp
#define strxfrm strncpy

/* BSDisms */
#define index strchr
#define rindex strrchr

/* Other common BSD functions */
extern int strcasecmp __P ((char*, char*));
extern int strncasecmp __P ((char*, char*, size_t));
char *strpbrk __P ((const char *, const char *));
char *strsep __P ((char **, char *));

char * strstr (const char *, const char *);

size_t strcspn __P ((char *, char *));
size_t strspn __P ((const char *, const char *));

/* Linux silly hour */
char *strfry __P ((char *));

void bzero (void * s, size_t n);

// TODO: this is removed in POSIX-1.2008
// TODO: replace by memcpy or memmove
#define bcopy(s, d, n) memcpy ((d), (s), (n))

void * memcpy (void * dest, const void * src, size_t n);

char *strcat (char * dest, const char * src);
size_t strlen (const char * s);
int strncmp (const char * s1, const char * s2, size_t n);
char *strtok (char * str, const char * delim);

int sprintf (char * str, const char * format, ...);

#endif
