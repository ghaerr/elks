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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/*

atan2( y : double, x : double ) : double - compute atan(y/x)

NOTE: x,y cannot both be 0

case 1: y = 0    if x < 0 return pi, otherwise return 0

case 2: x = 0    if y < 0 return -pi/2, otherwise return pi/2

otherwise: compute z = atan( y/x )
               if y >= 0 and z < 0 return z + pi
               if y <  0 and z > 0 return z - pi
               else return z

*/
#include "variety.h"
#include <stddef.h>
#include <float.h>
#include "mathlib.h"
#include "pi.h"
#include "ifprag.h"
#include "pdiv.h"


_WMRTLINK float _IF_atan2( float y, float x )
/*******************************************/
{
    return( _IF_datan2( y, x ) );
}

_WMRTLINK double (atan2)( double y, double x )
/********************************************/
{
    return( _IF_datan2( y, x ) );
}

_WMRTLINK double _IF_datan2( double y, double x )
/***********************************************/
{
    int     sgnx;   /* sgn(x) */
    int     sgny;   /* sgn(y) */
    int     sgnz;   /* sgn(z) */

    sgny = __sgn( y );
    sgnx = __sgn( x );
    if( sgny == 0 ) {               /* case 1 */
        if( sgnx == 0 ) {
            x = __math2err( FP_FUNC_ATAN2 | M_DOMAIN | V_ZERO, &y, &x );
        } else if( sgnx < 0 ) {
            x = PI;
        } else {
            x = 0.0;
        }
    } else if( sgnx == 0 ) {        /* case 2 */
        if( sgny < 0 ) {
            x = -PIby2;
        } else {
            x = PIby2;
        }
    } else {
        int exp_x;
        int exp_y;
        int diff_exp_y_x;

        frexp( y, &exp_y );
        frexp( x, &exp_x );

        diff_exp_y_x = exp_y - exp_x;

        if( diff_exp_y_x > DBL_MAX_EXP ) {
            if( sgnx == sgny ) {
                x = +DBL_MAX;
            } else {
                x = -DBL_MAX;
            }
        } else if( diff_exp_y_x < DBL_MIN_EXP ) {
            if( sgnx == sgny ) {
                x = +DBL_MIN;
            } else {
                x = -DBL_MIN;
            }
        } else {
            x = PDIV( y, x );
        }
        x = atan( x );
        sgnz = __sgn( x );
        if( sgny >= 0 ) {
            if( sgnz < 0 ) {
                x += PI;
            }
        } else {
            if( sgnz > 0 ) {
                x -= PI;
            }
        }
    }
    return( x );
}
