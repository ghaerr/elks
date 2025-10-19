/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  Square root routine.
*
****************************************************************************/


#include "variety.h"
#include <stddef.h>
#include "mathlib.h"
#include "ifprag.h"
//#include "rtdata.h"


_WMRTLINK float _IF_sqrt( float x )
/*********************************/
{
    return( _IF_dsqrt( x ) );
}

_WMRTLINK double sqrt( double x )
/*******************************/
{
    return( _IF_dsqrt( x ) );
}


_WMRTLINK double _IF_dsqrt( double x )
/************************************/
{
    if( x < 0.0 ) {
        x = __math1err( FP_FUNC_SQRT | M_DOMAIN | V_ZERO, &x );
#if defined(_M_IX86) && 0
    } else if( _RWD_real87 ) {
        x = __sqrt87( x );
#endif
    } else if( x != 0.0 ) {
#if defined(_M_IX86) && 0
        x = __sqrtd( x );
#else
        int         i;
        int         exp;
        double      e;

        x = frexp( x, &exp );
        if( exp & 1 ) {     /* if odd */
            ++exp;
            x = x / 2.0;
            e = x * 0.828427314758301 + 0.297334909439087;
        } else {        /* even */
            e = x * 0.58578634262085 + 0.42049503326416;
        }
        i = 4;
        do {
            e = (x / e + e) / 2.0;
        } while( --i != 0 );
        x = ldexp( e, exp >> 1 );
#endif
    }
    return( x );
}
