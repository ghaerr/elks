/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2024 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Declaration of non-standard functions and macros used by bootstrap
*
****************************************************************************/

#ifndef __WATCOMC__
/*
 * clibext.h:
 * This file contains defines and prototypes of functions that are present
 * in Watcom's CLIB but not in many other C libraries
 */
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef __BSD__
#include <unistd.h>         /* for off_t */
#else
#include <sys/types.h>      /* for off_t */
#endif
#ifdef __UNIX__
    #include <strings.h>    /* for str*case* functions */
    #include <sys/wait.h>
#endif

#define _WCRTLINK
#define _WCNEAR
#define _WCNORETURN
#define _WCI86NEAR
#define _WCI86FAR
#define _WCI86HUGE
#define __near
#define __based(x)

#if defined( __UNIX__ )

#ifndef P_WAIT
#define P_WAIT 0
#endif
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _MAX_PATH   (PATH_MAX+1)
#define _MAX_DRIVE  3
#define _MAX_DIR    (PATH_MAX-3)
#define _MAX_FNAME  (PATH_MAX-3)
#define _MAX_EXT    (PATH_MAX-3)
#define _fsopen(x,y,z) fopen(x,y)
#define _fmemcpy memcpy
#define __int64 long long
#ifndef _I32_MIN
#define _I32_MIN (-2147483647L-1L)
#endif
#ifndef _I32_MAX
#define _I32_MAX 2147483647L
#endif
#ifndef _UI32_MAX
#define _UI32_MAX 4294967295UL
#endif

#elif defined( _MSC_VER )

#define S_ISDIR(x) (((x)&_S_IFMT)==_S_IFDIR)
#define S_ISREG(x) (((x)&_S_IFMT)==_S_IFREG)
#define S_ISCHR(x) (((x)&_S_IFMT)==_S_IFCHR)
#define _A_VOLID 0
#define FNM_NOMATCH     1
#define FNM_NOESCAPE    0x01
#define FNM_PATHNAME    0x02
#define FNM_PERIOD      0x04
#define FNM_IGNORECASE  0x08
#define FNM_LEADING_DIR 0x10
#define NAME_MAX FILENAME_MAX
#define PATH_MAX FILENAME_MAX
#define _grow_handles _setmaxstdio

#endif

#define _MAX_PATH2 (_MAX_PATH + 3)

//#ifndef getch
//#define getch getchar
//#endif
#define __Strtold(s,ld,endptr) ((*(double *)(ld))=strtod(s,endptr))

#ifndef __cplusplus
#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif
#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined( __UNIX__ )

#elif defined( _MSC_VER )

typedef struct find_t {
    char            reserved[21];       /* reserved for use by DOS    */
    char            attrib;             /* attribute byte for file    */
    unsigned short  wr_time;            /* time of last write to file */
    unsigned short  wr_date;            /* date of last write to file */
    unsigned long   size;               /* length of file in bytes    */
    char            name[NAME_MAX+1];   /* null-terminated filename   */
} find_t;
typedef struct dirent {
    char            d_dta[21];          /* disk transfer area */
    char            d_attr;             /* file's attribute */
    unsigned short  d_time;             /* file's time */
    unsigned short  d_date;             /* file's date */
    long            d_size;             /* file's size */
    unsigned short  d_ino;              /* serial number (not used) */
    char            d_first;            /* flag for 1st time */
    char            *d_openpath;        /* path specified to opendir */
    char            d_name[NAME_MAX+1]; /* file's name */
} dirent;
typedef struct dirent   DIR;

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef long    ssize_t;
#endif
typedef int     mode_t;

#endif

#include "_pathgr2.h"

#if defined( __UNIX__ )

extern void     _makepath( char *path, const char *drive, const char *dir, const char *fname, const char *ext );
extern char     *_fullpath( char *buf, const char *path, size_t size );
extern char     *strlwr( char *string );
extern char     *strupr( char *string );
extern char     *strrev( char *string );
extern void     _searchenv( const char *name, const char *env_var, char *buf );
extern int      spawnlp( int mode, const char *path, const char *cmd, ... );
extern int      spawnvp( int mode, const char *cmd, const char * const *args );

#elif defined( _MSC_VER )

extern int      setenv( const char *name, const char *newvalue, int overwrite );
extern int      unsetenv( const char *name );
extern unsigned sleep( unsigned );

extern DIR              *opendir( const char *dirname );
extern struct dirent    *readdir( DIR *dirp );
extern int              closedir( DIR *dirp );

extern int      getopt( int argc, char * const argv[], const char *optstring );
/* Globals used and set by getopt() */
extern char     *optarg;
extern int      optind;
extern int      opterr;
extern int      optopt;

extern int      fnmatch( const char *__pattern, const char *__string, int __flags );
extern int      mkstemp( char *__template );

#endif

extern void     _splitpath2( const char *inp, char *outp, char **drive, char **dir, char **fn, char **ext );
extern int      _bgetcmd( char *buffer, int len );
extern char     *_cmdname( char *name );
extern char     *get_dllname( char *buf, int len );

#ifdef __cplusplus
}
#endif

#else   /* __WATCOMC__ */

/*
 * temporary fix for older builds of OW 2.0
 */
#if __WATCOMC__ == 1300
#ifndef _WCI86NEAR
#define _WCI86NEAR
#endif
#ifndef _WCI86FAR
#define _WCI86FAR
#endif
#ifndef _WCI86HUGE
#define _WCI86HUGE
#endif
#endif

#endif  /* !__WATCOMC__ */
