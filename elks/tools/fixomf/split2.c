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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <string.h>
#if !defined( __WIDECHAR__ ) && defined( CLIB_USE_MBCS_TRANSLATION )
    #include <mbstring.h>
#endif
#include "pathmac.h"


/* split full Unix path name into its components */

/* Under Unix we will map drive to node, dir to dir, and
 * filename to (filename and extension)
 *          or (filename) if no extension requested.
 */


static CHAR_TYPE *pcopy( CHAR_TYPE **pdst, CHAR_TYPE *dst, const CHAR_TYPE *b_src, const CHAR_TYPE *e_src )
/*=======================================================================================================*/
{
    unsigned    len;

    if( pdst == NULL )
        return( dst );
    *pdst = dst;
    len = e_src - b_src;
    if( len >= _MAX_PATH2 ) {
        len = _MAX_PATH2 - 1;
    }
#if defined( __WIDECHAR__ ) || !defined( CLIB_USE_MBCS_TRANSLATION )
    memcpy( dst, b_src, len * CHARSIZE );
    dst[len] = NULLCHAR;
    return( dst + len + 1 );
#else
    len = _mbsnccnt( (unsigned char *)b_src, len );                 /* # chars in len bytes */
    _mbsncpy( (unsigned char *)dst, (unsigned char *)b_src, len );  /* copy the chars */
    dst[_mbsnbcnt( (unsigned char *)dst, len )] = NULLCHAR;
    return( dst + _mbsnbcnt( (unsigned char *)dst, len ) + 1 );
#endif
}

_WCRTLINK void  __F_NAME(_splitpath2,_wsplitpath2)( CHAR_TYPE const *inp, CHAR_TYPE *outp,
                     CHAR_TYPE **drive, CHAR_TYPE **path, CHAR_TYPE **fn, CHAR_TYPE **ext )
/*=======================================================================================*/
{
    CHAR_TYPE const *dotp;
    CHAR_TYPE const *fnamep;
    CHAR_TYPE const *startp;
    UINT_WC_TYPE    ch;

    /* take apart specification like -> //0/hd/user/fred/filename.ext for QNX */
    /* take apart specification like -> \\disk2\fred\filename.ext for UNC names */
    /* take apart specification like -> c:\fred\filename.ext for DOS, OS/2 */

    /* process node/drive/UNC specification */
    startp = inp;
    if( IS_DIR_SEP( inp[0] ) && IS_DIR_SEP( inp[1] ) ) {
        inp += 2;
        for( ;; ) {
            if( *inp == NULLCHAR )
                break;
            if( IS_DIR_SEP( *inp ) )
                break;
            if( *inp == STRING( '.' ) )
                break;
#if defined( __WIDECHAR__ ) || !defined( CLIB_USE_MBCS_TRANSLATION )
            ++inp;
#else
            inp = (char *)_mbsinc( (unsigned char *)inp );
#endif
        }
        outp = pcopy( drive, outp, startp, inp );
#if defined( __NETWARE__ )
    /* process server/volume specification */
    } else if( (dotp = strchr( inp, DRV_SEP )) != NULL ) {
        inp = dotp + 1;
        outp = pcopy( drive, outp, startp, inp );
#elif !defined(__UNIX__)
    /* process drive specification */
    } else if( inp[0] != NULLCHAR && inp[1] == DRV_SEP ) {
        if( drive != NULL ) {
            *drive = outp;
            outp[0] = inp[0];
            outp[1] = DRV_SEP;
            outp[2] = NULLCHAR;
            outp += 3;
        }
        inp += 2;
#endif
    } else if( drive != NULL ) {
        *drive = outp;
        *outp = NULLCHAR;
        ++outp;
    }

    /* process /user/fred/filename.ext for QNX */
    /* process \fred\filename.ext for DOS, OS/2 */
    /* process /fred/filename.ext for DOS, OS/2 */
    dotp = NULL;
    fnamep = inp;
    startp = inp;

    for( ;; ) {
#if defined( __WIDECHAR__ ) || !defined( CLIB_USE_MBCS_TRANSLATION )
        ch = *inp;
#else
        ch = _mbsnextc( (unsigned char *)inp );
#endif
        if( ch == 0 )
            break;
        if( ch == EXT_SEP ) {
            dotp = inp;
            ++inp;
            continue;
        }
#if defined( __WIDECHAR__ ) || !defined( CLIB_USE_MBCS_TRANSLATION )
        inp++;
#else
        inp = (char *)_mbsinc( (unsigned char *)inp );
#endif
        if( IS_DIR_SEP( ch ) ) {
            fnamep = inp;
            dotp = NULL;
        }
    }
    outp = pcopy( path, outp, startp, fnamep );
    if( dotp == NULL )
        dotp = inp;
    outp = pcopy( fn, outp, fnamep, dotp );
    outp = pcopy( ext, outp, dotp, inp );
}
