/*
 *  linux/lib/string.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * stupid library routines.. The optimized versions should generally be found
 * as inline code in <asm-xx/string.h>
 *
 * These are buggy as well..
 */
 
#include <linuxmt/types.h>
#include <linuxmt/string.h>

char * ___strtok = NULL;

#ifndef __HAVE_ARCH_STRCPY
char * strcpy(dest,src)
char * dest;
char *src;
{
	register char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}
#endif

#ifndef __HAVE_ARCH_ATOI
int atoi(number)
char *number;
{
  int n=0,neg=0;

  while(*number <= ' ' && *number)
    ++number;
  if (!*number) return 0;
  if (*number == '-') { neg=1; number++; }
  else if (*number == '+') number++;
  while (*number>'/' && *number<':')
    n=(n*10)+(*number++)-'0';
  return(neg?-n:n);
}
#endif

#if 0
#ifndef __HAVE_ARCH_STRNCPY
char * strncpy(dest,src,count)
char * dest;
char *src;
size_t count;
{
	register char *tmp = dest;

	while (count-- && (*dest++ = *src++) != '\0')
		/* nothing */;

	return tmp;
}
#endif

#ifndef __HAVE_ARCH_STRCAT
char * strcat(dest,src)
char * dest;
char * src;
{
	register char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;

	return tmp;
}
#endif

#ifndef __HAVE_ARCH_STRNCAT
char * strncat(dest,src,count)
char *dest;
char *src;
size_t count;
{
	register char *tmp = dest;

	if (count) {
		while (*dest)
			dest++;
		while ((*dest++ = *src++)) {
			if (--count == 0) {
				*dest = '\0';
				break;
			}
		}
	}

	return tmp;
}
#endif
#endif

#ifndef __HAVE_ARCH_STRCMP
int strcmp(cs,ct)
register char * cs;
register char * ct;
{
	char __res;

	while (1) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
	}

	return __res;
}
#endif

#ifndef __HAVE_ARCH_STRNCMP
int strncmp(cs,ct,count)
register char * cs;
register char * ct;
size_t count;
{
	char __res = 0;

	while (count) {
		if ((__res = *cs - *ct++) != 0 || !*cs++)
			break;
		count--;
	}

	return __res;
}
#endif

#if 0
#ifndef __HAVE_ARCH_STRCHR
char * strchr(s,c)
char * s;
int c;
{
	for(; *s != (char) c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *) s;
}
#endif
#endif

#ifndef __HAVE_ARCH_STRLEN
size_t strlen(s)
char * s;
{
	register char *sc;

	for (sc = s; *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}
#endif

#ifndef __HAVE_ARCH_STRNLEN
size_t strnlen(s,count)
char * s;
size_t count;
{
	register char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}
#endif

#if 0
#ifndef __HAVE_ARCH_STRSPN
size_t strspn(s,accept)
char *s;
char *accept;
{
	register char *p;
	register char *a;
	size_t count = 0;

	for (p = s; *p != '\0'; ++p) {
		for (a = accept; *a != '\0'; ++a) {
			if (*p == *a)
				break;
		}
		if (*a == '\0')
			return count;
		++count;
	}

	return count;
}
#endif

#ifndef __HAVE_ARCH_STRPBRK
char * strpbrk(cs,ct)
char * cs;
char * ct;
{
	register char *sc1;
	register char *sc2;

	for( sc1 = cs; *sc1 != '\0'; ++sc1) {
		for( sc2 = ct; *sc2 != '\0'; ++sc2) {
			if (*sc1 == *sc2)
				return (char *) sc1;
		}
	}
	return NULL;
}
#endif

#ifndef __HAVE_ARCH_STRTOK
char * strtok(s,ct)
char * s;
char * ct;
{
	register char *sbegin, *send;

	sbegin  = s ? s : ___strtok;
	if (!sbegin) {
		return NULL;
	}
	sbegin += strspn(sbegin,ct);
	if (*sbegin == '\0') {
		___strtok = NULL;
		return( NULL );
	}
	send = strpbrk( sbegin, ct);
	if (send && *send != '\0')
		*send++ = '\0';
	___strtok = send;
	return (sbegin);
}
#endif
#endif

#ifndef __HAVE_ARCH_MEMSET
char * memset(s,c,count)
char * s;
char c;
size_t count;
{
	register char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}
#endif

#if 0
#ifndef __HAVE_ARCH_BCOPY
char * bcopy(src,dest,count)
char * src;
char * dest;
int count;
{
	register char *tmp = dest;

	while (count--)
		*tmp++ = *src++;

	return dest;
}
#endif
#endif

#ifndef __HAVE_ARCH_MEMCPY
char * memcpy(dest,src,count)
char * dest;
char *src;
size_t count;
{
	register char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}
#endif

#if 0
#ifndef __HAVE_ARCH_MEMMOVE
char * memmove(dest,src,count)
char * dest;
char *src;
size_t count;
{
	register char *tmp, *s;

	if (dest <= src) {
		tmp = (char *) dest;
		s = (char *) src;
		while (count--)
			*tmp++ = *s++;
		}
	else {
		tmp = (char *) dest + count;
		s = (char *) src + count;
		while (count--)
			*--tmp = *--s;
		}

	return dest;
}
#endif
#endif

#if 0
#ifndef __HAVE_ARCH_MEMCMP
int memcmp(cs,ct,count)
char * cs;
char * ct;
size_t count;
{
	register unsigned char *su1, *su2;
	char res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}
#endif

/*
 * find the first occurrence of byte 'c', or 1 past the area if none
 */
#ifndef __HAVE_ARCH_MEMSCAN
char * memscan(addr,c,size)
char * addr;
int c;
size_t size;
{
	register unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (char *) p;
		p++;
		size--;
	}
  	return (char *) p;
}
#endif

#ifndef __HAVE_ARCH_STRSTR
char * strstr(s1,s2)
char * s1;
char * s2;
{
	int l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	return NULL;
}
#endif
#endif
