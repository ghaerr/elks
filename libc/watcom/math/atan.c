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
* Description:  Arctangent routine.
*
****************************************************************************/


/*
        Uses the following identities
        *****************************

        arctan(x) = - arctan( - x )

        arctan(x) = pi/2 - arctan( 1/x )

        arctan(x) = arctan( (x*sqrt(3)-1) / (x+sqrt(3)) ) + pi/6
*/

#include "variety.h"
#include <float.h>
#include "mathlib.h"
#include "pi.h"
#include "ifprag.h"
//#include "rtdata.h"
#include "pdiv.h"

#define FALSE           0
#define TRUE            1

#define tan15           0.26794919243112270647
#define sqrt3           1.73205080756887729353
#define sqrt3m1         0.73205080756887729353


#define MIN_POLY_CONST 0.04
static  const double    AtanPoly[] = {
     0.0443895157187,
    -0.06483193510303,
     0.0767936869066,
    -0.0909037114191074,  //        -0.09090371144275426,
     0.11111097898051048,
    -0.14285714102825545,
     0.1999999999872945,
    -0.3333333333332993,
     1.0
};

_WMRTLINK float _IF_atan( float x )
/*********************************/
{
    return( _IF_datan( x ) );
}

_WMRTLINK double (atan)( double x )
/*********************************/
{
    return( _IF_datan( x ) );
}

_WMRTLINK double _IF_datan( double x )
/************************************/
{
    char    add_piby2;
    char    add_piby6;
    int     sgnx;
    double  tmp;

#if defined(_M_IX86) && 0
    if( _RWD_real87 )
        return( _atan87(x) );
#endif
    add_piby2 = FALSE;
    add_piby6 = FALSE;
    sgnx = __sgn( x );
    x = fabs( x );
    if( x == 1.0 ) {        /* 06-dec-88 */
        x = PIby4;
    } else if( x > (1.0 / DBL_MIN) ) {
        x = PIby2;
    } else if( x < (DBL_MIN / MIN_POLY_CONST) ) {
        x = DBL_MIN;
    } else {
        if( x > 1.0 ) {
            x = PDIV( 1.0, x );
            add_piby2 = TRUE;
        }
        if( x > tan15 ) {
            tmp = sqrt3m1 * x - 0.5;
            tmp -= 0.5;
            x = PDIV( (tmp + x), (x + sqrt3) );
//                x = (x * sqrt3 - 1.0) / (x + sqrt3);
            add_piby6 = TRUE;
        }
        x = _OddPoly( x, AtanPoly, 8 );
        if( add_piby6 ) {
            x += PIby6;
        }
        if( add_piby2 ) {
            x = PIby2 - x;
        }
    }
    if( sgnx < 0 ) {
        x = -x;
    }
    return( x );
}
