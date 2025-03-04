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
* Description:  Logarithm routine.
*
****************************************************************************/


#include "variety.h"
#include <stddef.h>
#include "mathlib.h"
#include "ifprag.h"
#include "rtdata.h"


#define sqrt_of_half    0.7071067811865475244
#define C1              (355./512.)
#define C2              (-2.121944400546905827679e-4)


static const double     APoly[] = {
    -0.78956112887491257267,
     0.16383943563021534222e+2,
    -0.64124943423745581147e+2
};

static const double     BPoly[] = {
     1.0,
    -0.35667977739034646171e+2,
     0.31203222091924532844e+3,
    -0.76949932108494879777e+3
};

_WMRTLINK float _IF_log( float x )
/********************************/
{
    return( _IF_dlog( x ) );
}

_WMRTLINK double (log)( double x )
/********************************/
{
    return( _IF_dlog( x ) );
}

_WMRTLINK double _IF_dlog( double x )
/***********************************/
{
    int     exp;
    double  z;

    if( x <= 0.0 ) {
        x = __log_matherr( x, FP_FUNC_LOG );
#if defined(_M_IX86)
    } else if( _RWD_real87 ) {
        x = _log87( x );
#endif
    } else {
        x = frexp( x, &exp );
        z = x - .5;
        if( x > sqrt_of_half ) {
            z -= .5;
        } else {
            x = z;
            --exp;
        }
        z = z / (x * .5 + .5);
        x = z * z;
        x = z + z * x * _EvalPoly( x, APoly, 2 ) / _EvalPoly( x, BPoly, 3 );
        if( exp != 0 ) {
            z = exp;
            x = (z * C2 + x) + z * C1;
        }
    }
    return( x );
}
