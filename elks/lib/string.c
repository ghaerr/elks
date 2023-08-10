/*
 *  linux/lib/string.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  12 Oct 21 ghaerr Add simple_strtol, remove old atoi
 */

/*
 * stupid library routines.. The optimized versions should generally be
 * found as inline code in <asm-xx/string.h>
 *
 * These are buggy as well..
 */

#include <linuxmt/types.h>
#include <linuxmt/string.h>

#define isdigit(c)	((c) >= '0' && (c) <= '9')
#define isalpha(c)	(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

long simple_strtol(const char *s, int base)
{
	register int c;
	int neg;
	long result = 0;

	/* Skip spaces and pick up leading +/- sign, if any */
	do {
		c = *s++;
	} while (c != '\0' && c <= ' ');
	if (((neg = c) == '-') || c == '+')
		c = *s++;

	/*
	 * If base is 0, allow 0x for hex and 0 for octal;
	 * if base is already 16, allow 0x.
	 */
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	do {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c = (c & 0xdf) - 'A' + 10;
		else break;
		if (c >= base)
			break;
		result = result * base + c;
	} while ((c = *s++) != '\0');

	return (neg == '-') ? -result: result;
}

int atoi(const char *number)
{
	return (int)simple_strtol(number, 10);
}

#ifndef __HAVE_ARCH_STRCPY

char *strcpy(char *dest, const char *src)
{
    register char *tmp = dest;

    while ((*tmp++ = *src++) != '\0')
	/* Do nothing */ ;

    return dest;
}

#endif

#if UNUSED

#ifndef __HAVE_ARCH_STRNCPY

char *strncpy(char *dest, const char *src, size_t count)
{
    register const char *tmp = dest;

    while (count-- && (*tmp++ = *src++))
	/* Do nothing */ ;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_STRCAT

char *strcat(char *dest, const char *src)
{
    register const char *tmp = dest;

    while (*tmp)
	tmp++;
    while ((*tmp++ = *src++))
	/* Do nothing */ ;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_STRNCAT

char *strncat(char *dest, const char *src, size_t count)
{
    register const char *tmp = dest;

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

int strcmp(register const char *cs, register const char *ct)
{
    while (*(cs++) != *(ct++))
	/* Do nothing */;
    return *(--cs) - *(--ct);
}

#endif

#ifndef __HAVE_ARCH_STRNCMP

int strncmp(register const char *s1, register const char *s2, size_t n)
{
    int r = 0;

    while (n--
	   && ((r = ((int)(*((const unsigned char *)s1))) - *((const unsigned char *)s2++))
	       == 0)
	   && *s1++);

    return r;
}

#endif

#ifndef __HAVE_ARCH_STRLEN

size_t strlen(const char *s)
{
    register const char *sc;

    for (sc = s; *sc; ++sc)
	/* Do nothing */ ;
    return sc - s;
}

#endif

#ifndef __HAVE_ARCH_STRNLEN

size_t strnlen(const char *s, size_t max)
{
    register const char *p = s;
    size_t maxp = max;

    while (maxp && *p) {
	++p;
	--maxp;
    }

    return p - s;
}

#endif

#ifndef __HAVE_ARCH_MEMSET

void *memset(void *s, int c, size_t count)
{
    register char *xs = s;

    while (count--)
	*xs++ = c;

    return s;
}

#endif

#ifndef __HAVE_ARCH_MEMCPY

void *memcpy(void *dest, const void *src, size_t count)
{
    register char *tmp = dest;
    register const char *s = src;

    while (count--)
	*tmp++ = *s++;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_MEMCMP

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *su1 = s1;
    const unsigned char *su2 = s2;
    char res = 0;

    for (; n > 0; n--)
        if ((res = *su1++ - *su2++))
            break;

    return res;
}

#endif

#ifndef __HAVE_ARCH_STRCHR

char *strchr(const char *s, int c)
{
    for(; *s != (char)c; ++s) {
        if (*s == '\0')
            return NULL;
    }
    return (char *)s;
}

#endif

#if UNUSED

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
    while (*number >= '0' && *number <= '9')
	n = (n * 10) + (*number++ - '0');
    return (neg == '-' ? -n : n);
}

#endif

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

char *___strtok = NULL;

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

#ifndef __HAVE_ARCH_BCOPY

char *bcopy(char *src, char *dest, int count)
{
    register char *tmp = dest;

    while (count--)
	*tmp++ = *src++;

    return dest;
}

#endif

#ifndef __HAVE_ARCH_MEMMOVE

void *memmove(void *dest, const void *src, size_t count)
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
