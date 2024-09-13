/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
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
* Description:  Platform independent _makepath() implementation.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <string.h>
#if defined( CLIB_USE_MBCS_TRANSLATION )
    #include <mbstring.h>
#endif
#include "pathmac.h"

#undef _makepath


#if defined(__UNIX__)

/* create full Unix style path name from the components */

_WCRTLINK void __F_NAME(_makepath,_wmakepath)(
        CHAR_TYPE           *path,
        const CHAR_TYPE  *node,
        const CHAR_TYPE  *dir,
        const CHAR_TYPE  *fname,
        const CHAR_TYPE  *ext )
{
    *path = NULLCHAR;

    if( node != NULL ) {
        if( *node != NULLCHAR ) {
            __F_NAME(strcpy,wcscpy)( path, node );
            path = __F_NAME(strchr,wcschr)( path, NULLCHAR );

            /* if node did not end in '/' then put in a provisional one */
            if( path[-1] == DIR_SEP ) {
                path--;
            } else {
                *path = DIR_SEP;
            }
        }
    }
    if( dir != NULL ) {
        if( *dir != NULLCHAR ) {
            /*  if dir does not start with a '/' and we had a node then
                    stick in a separator
            */
            if( ( *dir != DIR_SEP ) && ( *path == DIR_SEP ) )
                path++;

            __F_NAME(strcpy,wcscpy)( path, dir );
            path = __F_NAME(strchr,wcschr)( path, NULLCHAR );

            /* if dir did not end in '/' then put in a provisional one */
            if( path[-1] == DIR_SEP ) {
                path--;
            } else {
                *path = DIR_SEP;
            }
        }
    }

    if( fname != NULL && *fname != NULLCHAR ) {
        if( ( *fname != DIR_SEP ) && ( *path == DIR_SEP ) )
            path++;

        __F_NAME(strcpy,wcscpy)( path, fname );
        path = __F_NAME(strchr,wcschr)( path, NULLCHAR );
    } else {
        if( *path == DIR_SEP ) {
            path++;
        }
    }
    if( ext != NULL ) {
        if( *ext != NULLCHAR ) {
            if( *ext != EXT_SEP )
                *path++ = EXT_SEP;
            __F_NAME(strcpy,wcscpy)( path, ext );
            path = __F_NAME(strchr,wcschr)( path, NULLCHAR );
        }
    }
    *path = NULLCHAR;
}

#elif defined( __NETWARE__ )

/*
    For silly two choice DOS path characters / and \,
    we want to return a consistent path character.
*/

static char pickup( char c, char *pc_of_choice )
{
    if( IS_DIR_SEP( c ) ) {
        if( *pc_of_choice == NULLCHAR )
            *pc_of_choice = c;
        c = *pc_of_choice;
    }
    return( c );
}

_WCRTLINK void _makepath( char *path, const char *volume,
                const char *dir, const char *fname, const char *ext )
{
    char first_pc = NULLCHAR;

    if( volume != NULL ) {
        if( *volume != NULLCHAR ) {
            do {
                *path++ = *volume++;
            } while( *volume != NULLCHAR );
            if( path[-1] != DRV_SEP ) {
                *path++ = DRV_SEP;
            }
        }
    }
    *path = NULLCHAR;
    if( dir != NULL ) {
        if( *dir != NULLCHAR ) {
            do {
                *path++ = pickup( *dir++, &first_pc );
            } while( *dir != NULLCHAR );
            /* if no path separator was specified then pick a default */
            if( first_pc == NULLCHAR )
                first_pc = DIR_SEP;
            /* if dir did not end in path sep then put in a provisional one */
            if( path[-1] == first_pc ) {
                path--;
            } else {
                *path = first_pc;
            }
        }
    }
    /* if no path separator was specified thus far then pick a default */
    if( first_pc == NULLCHAR )
        first_pc = DIR_SEP;
    if( fname != NULL && *fname != NULLCHAR ) {
        if( ( pickup( *fname, &first_pc ) != first_pc ) && ( *path == first_pc ) )
            path++;
        while( *fname != NULLCHAR ) {
            *path++ = pickup( *fname++, &first_pc );
        }
    } else {
        if( *path == first_pc ) {
            path++;
        }
    }
    if( ext != NULL ) {
        if( *ext != NULLCHAR ) {
            if( *ext != EXT_SEP )
                *path++ = EXT_SEP;
            while( *ext != NULLCHAR ) {
                *path++ = *ext++;
            }
        }
    }
    *path = NULLCHAR;
}

#else

/*
    For silly two choice DOS path characters / and \,
    we want to return a consistent path character.
*/

static UINT_WC_TYPE pickup( UINT_WC_TYPE c, UINT_WC_TYPE *pc_of_choice )
{
    if( IS_DIR_SEP( c ) ) {
        if( *pc_of_choice == NULLCHAR )
            *pc_of_choice = c;
        c = *pc_of_choice;
    }
    return( c );
}

/* create full MS-DOS path name from the components */

_WCRTLINK void __F_NAME(_makepath,_wmakepath)( CHAR_TYPE *path, const CHAR_TYPE *drive,
                const CHAR_TYPE *dir, const CHAR_TYPE *fname, const CHAR_TYPE *ext )
{
    UINT_WC_TYPE        first_pc = NULLCHAR;
  #if !defined( __WIDECHAR__ ) && !defined( __RDOS__ )
    char                *pathstart = path;
    unsigned            ch;
  #endif

    if( drive != NULL ) {
        if( *drive != NULLCHAR ) {
            if( ( drive[0] == DIR_SEP ) && ( drive[1] == DIR_SEP ) ) {
                __F_NAME(strcpy, wcscpy)( path, drive );
                path += __F_NAME(strlen, wcslen)( drive );
            } else {
                *path++ = *drive;                               /* OK for MBCS */
                *path++ = DRV_SEP;
            }
        }
    }
    *path = NULLCHAR;
    if( dir != NULL ) {
        if( *dir != NULLCHAR ) {
            do {
  #if defined( __WIDECHAR__ ) || defined( __RDOS__ )
                *path++ = pickup( *dir++, &first_pc );
  #else
                ch = pickup( _mbsnextc( (unsigned char *)dir ), &first_pc );
                _mbvtop( ch, (unsigned char *)path );
                path[_mbclen( (unsigned char *)path )] = NULLCHAR;
                path = (char *)_mbsinc( (unsigned char *)path );
                dir = (char *)_mbsinc( (unsigned char *)dir );
  #endif
            } while( *dir != NULLCHAR );
            /* if no path separator was specified then pick a default */
            if( first_pc == NULLCHAR )
                first_pc = DIR_SEP;
            /* if dir did not end in '/' then put in a provisional one */
  #if defined( __WIDECHAR__ ) || defined( __RDOS__ )
            if( path[-1] == first_pc ) {
                path--;
            } else {
                *path = first_pc;
            }
  #else
            if( *(_mbsdec( (unsigned char *)pathstart, (unsigned char *)path )) == first_pc ) {
                path--;
            } else {
                *path = first_pc;
            }
  #endif
        }
    }

    /* if no path separator was specified thus far then pick a default */
    if( first_pc == NULLCHAR )
        first_pc = DIR_SEP;
    if( fname != NULL && *fname != NULLCHAR ) {
  #if defined( __WIDECHAR__ ) || defined( __RDOS__ )
        if( pickup( *fname, &first_pc ) != first_pc && *path == first_pc )
            path++;
  #else
        ch = _mbsnextc( (unsigned char *)fname );
        if( pickup( ch, &first_pc ) != first_pc && *path == first_pc )
            path++;
  #endif

        while( *fname != NULLCHAR ) {
        //do {
  #if defined( __WIDECHAR__ ) || defined( __RDOS__ )
            *path++ = pickup( *fname++, &first_pc );
  #else
            ch = pickup( _mbsnextc( (unsigned char *)fname ), &first_pc );
            _mbvtop( ch, (unsigned char *)path );
            path[_mbclen( (unsigned char *)path )] = NULLCHAR;
            path = (char *)_mbsinc( (unsigned char *)path );
            fname = (char *)_mbsinc( (unsigned char *)fname );
  #endif
        } //while( *fname != NULLCHAR );
    } else {
        if( *path == first_pc ) {
            path++;
        }
    }
    if( ext != NULL ) {
        if( *ext != NULLCHAR ) {
            if( *ext != EXT_SEP )
                *path++ = EXT_SEP;
            while( *ext != NULLCHAR ) {
                *path++ = *ext++;     /* OK for MBCS */
            }
        }
    }
    *path = NULLCHAR;
}
#endif
