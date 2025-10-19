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
* Description:  Rounds the argument to the nearest integer, always rounding
*               halfway away from zero
*
* Notes: This function should throw an exception if the result of rounding 
*        will be either inexact or overflow the double.  It currently does 
*        neither.
*
* Author: J. Armstrong
*
****************************************************************************/

#include <assert.h>
#include "variety.h"
#include <math.h>
#include "mathlib.h"
//#include "_matherr.h"

_WMRTLINK double round(double x)
{
double y;

    if(isnan(x) || isinf(x)) {
        __reporterror(DOMAIN, __func__, x, 0, x);
    }

    y = fabs(x);
    y = y + 0.5;
    
    if( isinf(y) ) {
        __reporterror(DOMAIN, __func__, x, 0, x);
        return x;
    }
    
    y = floor(y);
    return copysign(y, x);
}
