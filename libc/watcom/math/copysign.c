/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 2014 Open Watcom contributors. 
*    All Rights Reserved.
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
*    Based on FDLIBM
*    Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
*
*    Developed at SunSoft, a Sun Microsystems, Inc. business.
*    Permission to use, copy, modify, and distribute this
*    software is freely granted, provided that this notice 
*    is preserved.
*
*  ========================================================================
*
* Description:  Returns a value with the magnitude of x and
*               with the sign bit of y.
*
****************************************************************************/

#include "variety.h"
#include <math.h>
#include "xfloat.h"

_WMRTLINK double copysign(double x, double y)
{
    float_double fdx;
    float_double fdy;
    
    fdx.u.value = x;
    fdy.u.value = y;
    
    fdx.u.word[1] = (fdx.u.word[1] & 0x7fffffff) |
                    (fdy.u.word[1] & 0x80000000);
    
    return fdx.u.value;
}
