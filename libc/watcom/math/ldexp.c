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
* Description:  Base 2 exponentiation routine.
*
****************************************************************************/


#include "variety.h"
#include <math.h>
#include "mathlib.h"
//#include "clibsupp.h"


_WMRTLINK double ldexp( double value, int n )
/*******************************************/
{
    int     exp;
    union {
        double      x;
        short       a[4];
    } u;

    u.x = value;
    if( value != 0.0 ) {
        exp = (u.a[3] & 0x7ff0) >> 4;
        if( n > 16000 ) n = 16000;  /* so exp +=n does not overflow */
        if( n < -16000 ) n = -16000;/* so exp +=n does not underflow */
        exp += n;
        if( exp <= 0 ) {
            return( 0.0 );
        }
        if( exp >= 0x07ff ) {
            __set_ERANGE();
            if( u.a[3] > 0 ) {
                return( HUGE_VAL );
            }
            return( -HUGE_VAL );
        }
        exp = exp << 4;
        u.a[3] &= 0x800f;
        u.a[3] |= exp;
    }
    return( u.x );
}
