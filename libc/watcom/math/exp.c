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
* Description:  Floating-point exponentiation routine.
*
****************************************************************************/


#include "variety.h"
#include <stddef.h>
#include "mathlib.h"
#include "ifprag.h"
#include "rtdata.h"


#define log2e           1.4426950408889633
#define const0          20.813771196523035
#define const1          0.057761135831801924
#define const2          7.213503410844819


static  const double    ExpConsts[] = {
    1.0442737824274138403,  /*   sqrt(sqrt(sqrt(sqrt(2))))  */
    1.0905077326652576592,  /*   sqrt(sqrt(sqrt(2)))        */
    1.1892071150027210667,  /*   sqrt(sqrt(2))              */
    1.4142135623730950488   /*   sqrt(2)                    */
};

_WMRTLINK float _IF_exp( float x )
/********************************/
{
    return( _IF_dexp( x ) );
}

_WMRTLINK double (exp)( double x )
/********************************/
{
    return( _IF_dexp( x ) );
}

_WMRTLINK double _IF_dexp( double x )
/***********************************/
{
    int                 sgnx;
    int                 exp;
    int                 exp2;
    const double        *poly;
    double              ipart;
    double              a;
    double              b;
    double              ee;

    if( fabs( x ) < 4.445e-16 ) {       /* if argument is too small */
        x = 1.0;
    } else if( fabs( x ) > 709.782712893384 ) {/* if argument is too large */
        if( x < 0.0 ) {
            /* FPStatus = FPS_UNDERFLOW;   - - underflow */
            x = 0.0;                    /* - - set result to 0 */
        } else {
            x = __math1err( FP_FUNC_EXP | M_OVERFLOW | V_HUGEVAL, &x );
        }
#if defined(_M_IX86)
    } else if( _RWD_real87 ) {
        x = _exp87( x );
#endif
    } else {
        x *= log2e;
        sgnx = __sgn( x );
        x = modf( fabs( x ), &ipart );
        exp = ipart;
        if( sgnx < 0 ) {
            exp = -exp;
            if( x != 0.0 ) {
                --exp;
                x = 1.0 - x;
            }
        }
        exp2 = 0;
        if( x != 0.0 ) {
            x = modf( ldexp( x, 4 ), &ipart );
            if( x != 0.0 ) {
                x = ldexp( x, -4 );
            }
            exp2 = ipart;
        }
        ee = x * x;
        a = ee + const0;
        b = (ee * const1 + const2) * x;
        x = (a + b) / (a - b);
        poly = ExpConsts;
        while( exp2 != 0 ) {
            if( exp2 & 1 ) {
                x *= *poly;
            }
            ++poly;
            exp2 = exp2 >> 1;
        }
        x = ldexp( x, exp );
    }
    return( x );
}
