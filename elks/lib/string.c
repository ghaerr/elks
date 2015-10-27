/*
 *  linux/lib/string.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * stupid library routines.. The optimized versions should generally be
 * found as inline code in <asm-xx/string.h>
 *
 * These are buggy as well..
 */

#include <linuxmt/types.h>
#include <linuxmt/string.h>

char *___strtok = NULL;

#ifndef __HAVE_ARCH_STRCPY

char *strcpy(char *dest, char *src)
{
    register char *tmp = dest;

    while ((*tmp++ = *src++) != '\0')
	/* Do nothing */ ;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_ATOI

int atoi(register char *number)
{
    int n = 0;
    char neg;

/*      if (!number) */
/*  	return 0; */
    while (*number <= ' ') {
	if (!*number++)
	    return 0;
    }
    if (((neg = *number) == '-') || (*number == '+')) {
	++number;
    }
    while ((*number - '0') <= 9) {
	n = (n * 10) + (*number++ - '0');
    }
    return (neg == '-' ? -n : n);
}

#endif

#if 0

#ifndef __HAVE_ARCH_STRNCPY

char *strncpy(char *dest, char *src, size_t count)
{
    register char *tmp = dest;

    while (count-- && (*tmp++ = *src++))
	/* Do nothing */ ;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_STRCAT

char *strcat(char *dest, char *src)
{
    register char *tmp = dest;

    while (*tmp)
	tmp++;
    while ((*tmp++ = *src++))
	/* Do nothing */ ;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_STRNCAT

char *strncat(char *dest, char *src, size_t count)
{
    register char *tmp = dest;

    if (count) {
	while (*tmp)
	    tmp++;
	while ((*tmp++ = *src++) && --count)
	    /* Do nothing */;
	*tmp = '\0';
    }

    return dest;
}

#endif

#endif

#ifndef __HAVE_ARCH_STRCMP

int strcmp(register char *cs, register char *ct)
{
    while (*(cs++) != *(ct++))
	/* Do nothing */;
    return *(--cs) - *(--ct);
}

#endif

#ifndef __HAVE_ARCH_STRNCMP

int strncmp(register char *s1, register char *s2, size_t n)
{
    int r = 0;

    while (n--
	   && ((r = ((int)(*((unsigned char *)s1))) - *((unsigned char *)s2++))
	       == 0)
	   && *s1++);

    return r;
}

#endif

#if 0

#ifndef __HAVE_ARCH_STRCHR

char *strchr(char *s, int c)
{
    while (*s) {
	if (*s++ == (char) c)
	    return s;
    return NULL;
}

#endif

#endif

#ifndef __HAVE_ARCH_STRLEN

size_t strlen(char *s)
{
    register char *sc;

    for (sc = s; *sc; ++sc)
	/* Do nothing */ ;
    return sc - s;
}

#endif

#ifndef __HAVE_ARCH_STRNLEN

size_t strnlen(char *s, size_t max)
{
    register char *p = s;
    register char *maxp = (char *) max;

    while (maxp && *p) {
	++p;
	--maxp;
    }

    return p - s;
}

#endif

#if 0

#ifndef __HAVE_ARCH_STRSPN

size_t strspn(char *s, char *accept)
{
    register char *p, *a;
    size_t count = 0;

    for (p = s; *p && *a; ++p) {
	for (a = accept; *a; ++a)
	    if (*p == *a)
		break;
	++count;
    }

    return count;
}

#endif

#ifndef __HAVE_ARCH_STRPBRK

char *strpbrk(char *cs, char *ct)
{
    register char *sc1, *sc2;

    for (sc1 = cs; *sc1 != '\0'; ++sc1) {
	for (sc2 = ct; *sc2 != '\0'; ++sc2) {
	    if (*sc1 == *sc2)
		return (char *) sc1;
	}
    }
    return NULL;
}

#endif

#ifndef __HAVE_ARCH_STRTOK

char *strtok(char *s, char *ct)
{
    register char *sbegin, *send;

    sbegin = s ? s : ___strtok;
    if (!sbegin) {
	return NULL;
    }
    sbegin += strspn(sbegin, ct);
    if (*sbegin == '\0') {
	___strtok = NULL;
	return NULL;
    }
    send = strpbrk(sbegin, ct);
    if (send && *send != '\0')
	*send++ = '\0';
    ___strtok = send;
    return sbegin;
}

#endif

#endif

#ifndef __HAVE_ARCH_MEMSET

void *memset(void *s, char c, size_t count)
{
    register char *xs = s;

    while (count--)
	*xs++ = c;

    return s;
}

#endif

#if 0

#ifndef __HAVE_ARCH_BCOPY

char *bcopy(char *src, char *dest, int count)
{
    register char *tmp = dest;

    while (count--)
	*tmp++ = *src++;

    return dest;
}

#endif

#endif

#ifndef __HAVE_ARCH_MEMCPY

void *memcpy(void *dest, void *src, size_t count)
{
    register char *tmp = dest, *s = src;

    while (count--)
	*tmp++ = *s++;

    return dest;
}

#endif

#if 0

#ifndef __HAVE_ARCH_MEMMOVE

void *memmove(void *dest, void *src, size_t count)
{
    register char *tmp = dest, *s = src;

    if (dest <= src) {
	while (count--)
	    *tmp++ = *s++;
    } else {
	tmp += count;
	s += count;
	while (count--)
	    *(--tmp) = *(--s);
    }

    return dest;
}

#endif

#endif

#if 0

#ifndef __HAVE_ARCH_MEMCMP

int memcmp(void *cs, void *ct, size_t count)
{
    register unsigned char *su1 = cs, *su2 = ct;
    char res = 0;

    for (; count > 0; count--)
	if (!(res = *su1++ - *su2++))
	    break;

    return res;
}

#endif

/*
 * find the first occurrence of byte 'c', or 1 past the area if none
 */
#ifndef __HAVE_ARCH_MEMSCAN

void *memscan(void *addr, int c, size_t size)
{
    register unsigned char *p = (unsigned char *) addr;

    while (size-- && *p != c) {
	p++;
    }
    return (char *) p;
}

#endif

#ifndef __HAVE_ARCH_STRSTR

char *strstr(char *s1, char *s2)
{
    int l1, l2 = strlen(s2);

    if (!l2)
	return (char *) s1;

    l1 = strlen(s1);
    while (l1-- >= l2) {
	if (!memcmp(s1, s2, l2))
	    return s1;
	s1++;
    }
    return NULL;
}

#endif

#endif
