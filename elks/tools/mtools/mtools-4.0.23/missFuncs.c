/*  Copyright 1991 Free Software Foundation, Inc.
 *  Copyright 1997,1999-2002,2007-2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "sysincludes.h"
#include "mtools.h"

#ifndef HAVE_STRDUP


char *strdup(const char *str)
{
    char *nstr;

    if (str == (char*)0)
        return 0;

    nstr = (char*)malloc((strlen(str) + 1));

    if (nstr == (char*)0)
    {
        (void)fprintf(stderr, "strdup(): not enough memory to duplicate `%s'\n",
		      str);
	exit(1);
    }

    (void)strcpy(nstr, str);

    return nstr;
}
#endif /* HAVE_STRDUP */

#ifndef HAVE_STRNDUP
char *strndup( const char *s, size_t n )
{
    size_t nAvail;
    char *p;

    if ( !s )
        return 0;

    nAvail = min( strlen(s) + 1, n + 1 );
    p      = malloc( nAvail );
    if ( !p )
	    return 0;
    memcpy( p, s, nAvail );
    p[nAvail - 1] = '\0';

    return p;
}
#endif /* HAVE_STRNDUP */


#ifdef HAVE_WCHAR_H
#ifndef HAVE_WCSDUP
wchar_t *wcsdup(const wchar_t *wcs)
{
    wchar_t *nwcs;

    if (wcs == (wchar_t*)0)
        return 0;

    nwcs = (wchar_t*)calloc(wcslen(wcs) + 1, sizeof(wchar_t));

    if (nwcs == (wchar_t*)0)
    {
        (void)fprintf(stderr, "wcsdup(): not enough memory to duplicate `%ls'\n",
		      wcs);
	exit(1);
    }

    (void)wcscpy(nwcs, wcs);

    return nwcs;
}
#endif /* HAVE_WCSDUP */
#endif

#ifndef HAVE_MEMCPY
/*
 * Copy contents of memory (with possible overlapping).
 */
char *memcpy(char *s1, const char *s2, size_t n)
{
	bcopy(s2, s1, n);
	return(s1);
}
#endif

#ifndef HAVE_MEMSET
/*
 * Copies the character c, n times to string s
 */
char *memset(char *s, char c, size_t n)
{
	char *s1 = s;

	while (n > 0) {
		--n;
		*s++ = c;
	}
	return(s1);
}
#endif /* HAVE_MEMSET */


#ifndef HAVE_STRCHR

char * strchr (const char* s, int c)
{
	if (!s) return NULL;
	while (*s && *s != c) s++;
	if (*s) 
		return (char*) s;
	else
		return NULL;
}

#endif

#ifndef HAVE_STRRCHR

char * strrchr (const char* s1, int c) 
{
	char* s = (char*) s1;
	char* start = (char*) s;
	if (!s) return NULL;
	s += strlen(s)-1;
	while (*s != c && (unsigned long) s != (unsigned long) start) s--;
	if ((unsigned long) s == (unsigned long) start && *s != c)
		return NULL;
	else
		return s;
}

#endif

#ifndef HAVE_STRPBRK
/*
 * Return ptr to first occurrence of any character from `brkset'
 * in the character string `string'; NULL if none exists.
 */
char *strpbrk(const char *string, const char *brkset)
{
	register char *p;

	if (!string || !brkset)
		return(0);
	do {
		for (p = brkset; *p != '\0' && *p != *string; ++p)
			;
		if (*p != '\0')
			return(string);
	}
	while (*string++);
	return(0);
}
#endif /* HAVE_STRPBRK */


#ifndef HAVE_STRTOUL
static int getdigit(char a, int max)
{
	int dig;
	
	if(a < '0')
		return -1;
	if(a <= '9') {
		dig = a - '0';
	} else if(a >= 'a')
		dig = a - 'a' + 10;
	else if(a >= 'A')
		dig = a - 'A' + 10;
	if(dig >= max)
		return -1;
	else
		return dig;
}

unsigned long strtoul(const char *string, char **eptr, int base)
{
	int accu, dig;

	if(base < 1 || base > 36) {
		if(string[0] == '0') {
			switch(string[1]) {
			       	case 'x':
				case 'X':
					return strtoul(string+2, eptr, 16);
				case 'b':
			       	case 'B':
					return strtoul(string+2, eptr, 2);
				default:
					return strtoul(string, eptr, 8);
			}
		}
	       	return strtoul(string, eptr, 10);
	}
	if(base == 16 && string[0] == '0' &&
	   (string[1] == 'x' || string[1] == 'X'))
		string += 2;

	if(base == 2 && string[0] == '0' &&
	   (string[1] == 'b' || string[1] == 'B'))
		string += 2;
	accu = 0;
	while( (dig = getdigit(*string, base)) != -1 ) {
		accu = accu * base + dig;
		string++;
	}
	if(eptr)
		*eptr = (char *) string;
	return accu;
}
#endif /* HAVE_STRTOUL */

#ifndef HAVE_STRTOL
long strtol(const char *string, char **eptr, int base)
{
	long l;

	if(*string == '-') {
		return -(long) strtoul(string+1, eptr, base);
	} else {
		if (*string == '+')
			string ++;
		return (long) strtoul(string, eptr, base);
	}
}
#endif



#ifndef HAVE_STRSPN
/* Return the length of the maximum initial segment
   of S which contains only characters in ACCEPT.  */
size_t strspn(const char *s, const char *accept)
{
  register char *p;
  register char *a;
  register size_t count = 0;

  for (p = s; *p != '\0'; ++p)
    {
      for (a = accept; *a != '\0'; ++a)
	if (*p == *a)
	  break;
      if (*a == '\0')
	return count;
      else
	++count;
    }

  return count;
}
#endif /* HAVE_STRSPN */

#ifndef HAVE_STRCSPN
/* Return the length of the maximum initial segment of S
   which contains no characters from REJECT.  */
size_t strcspn (const char *s, const char *reject)
{
  register size_t count = 0;

  while (*s != '\0')
    if (strchr (reject, *s++) == NULL)
      ++count;
    else
      return count;

  return count;
}

#endif /* HAVE_STRCSPN */

#ifndef HAVE_STRERROR

#ifndef DECL_SYS_ERRLIST
extern char *sys_errlist[];
#endif

char *strerror(int errno)
{
  return sys_errlist[errno];
}
#endif

#ifndef HAVE_STRCASECMP
/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexiographically less than,
   equal to or greater than S2.  */
int strcasecmp(const char *s1, const char *s2)
{
  register const unsigned char *p1 = (const unsigned char *) s1;
  register const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      c1 = tolower (*p1++);
      c2 = tolower (*p2++);
      if (c1 == '\0')
	break;
    }
  while (c1 == c2);

  return c1 - c2;
}
#endif

#ifdef HAVE_WCHAR_H
#ifndef HAVE_WCSCASECMP
/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexiographically less than,
   equal to or greater than S2.  */
int wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
  register const wchar_t *p1 = s1;
  register const wchar_t *p2 = s2;
  wchar_t c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      c1 = towlower (*p1++);
      c2 = towlower (*p2++);
      if (c1 == '\0')
	break;
    }
  while (c1 == c2);

  return c1 - c2;
}
#endif
#endif


#ifndef HAVE_STRCASECMP
/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexiographically less than,
   equal to or greater than S2.  */
int strncasecmp(const char *s1, const char *s2, size_t n)
{
  register const unsigned char *p1 = (const unsigned char *) s1;
  register const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  c1 = c2 = 1;
  while (c1 && c1 == c2 && n-- > 0)
    {
      c1 = tolower (*p1++);
      c2 = tolower (*p2++);
    }

  return c1 - c2;
}
#endif

#ifndef HAVE_GETPASS
char *getpass(const char *prompt)
{
	static char password[129];
	int l;

	fprintf(stderr,"%s",prompt);
	fgets(password, 128, stdin);
	l = strlen(password);
	if(l && password[l-1] == '\n')
		password[l-1] = '\0';
	return password;

}
#endif

#ifndef HAVE_ATEXIT

#ifdef HAVE_ON_EXIT
int atexit(void (*function)(void))
{
	return on_exit( (void(*)(int,void*)) function, 0);
}
#else

typedef struct exitCallback {
	void (*function) (void);
	struct exitCallback *next;
} exitCallback_t;

static exitCallback_t *callback = 0;

int atexit(void (*function) (void))
{
	exitCallback_t *newCallback;
		
	newCallback = New(exitCallback_t);
	if(!newCallback) {
		printOom();
		exit(1);
	}
	newCallback->function = function;
	newCallback->next = callback;
	callback = newCallback;
	return 0;
}
#undef exit

void myexit(int code)
{
  void (*function)(void);

  while(callback) {
    function = callback->function;
    callback = callback->next;
    function();
  }
  exit(code);
}

#endif

#endif

static const char PATH_SEP = '/';

/*#ifndef HAVE_BASENAME*/
const char *_basename(const char *filename)
{
	char *ptr;

	ptr = strrchr(filename, PATH_SEP);
	if(ptr)
		filename = ptr + 1;

#ifdef OS_mingw32msvc
	ptr = strrchr(filename, '\\');
	if(ptr)
		filename = ptr + 1;
#endif

	return filename;
}
/*#endif*/

/* Strip the suffix ".exe" from the argument, if present. */
void _stripexe(char *filename)
{
	char *ptr;
	ptr = strrchr(filename, '.');
	if(ptr && !strcasecmp(ptr, ".exe"))
		*ptr = '\0';
}

#ifndef HAVE_STRNLEN
size_t strnlen(const char *str, size_t l)
{
  size_t i;
  for(i=0; i<l; i++) {
    if(str[i] == 0)
      break;
  }
  return i;
}
#endif /* HAVE_STRNLEN */

#ifdef HAVE_WCHAR_H
#ifndef HAVE_WCSNLEN
size_t wcsnlen(const wchar_t *wcs, size_t l)
{
  size_t i;
  for(i=0; i<l; i++) {
    if(wcs[i] == 0)
      break;
  }
  return i;
}
#endif /* HAVE_WCSNLEN */
#endif
