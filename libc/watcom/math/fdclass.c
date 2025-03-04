/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2024      The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Determine class of an IEEE 754 double.
*
****************************************************************************/


#include "variety.h"
#include <math.h>
#include "xfloat.h"


_WMRTLINK int _FDClass( double x )
/********************************/
{
    float_double    fd;

    fd.u.value = x;
    if( (fd.u.word[1] & 0x7FF00000) == 0x7FF00000 ) {    /* NaN or Inf */
        if( (fd.u.word[1] & 0x7FFFFFFF) == 0x7FF00000 && fd.u.word[0] == 0 ) {
            return( FP_INFINITE );
        }
        return( FP_NAN );
    }
    if( (fd.u.word[1] & 0x7FFFFFFF) == 0 && fd.u.word[0] == 0 ) {
        return( FP_ZERO );
    }
    if( (fd.u.word[1] & 0x7FF00000) == 0 ) {
        return( FP_SUBNORMAL );
    }
    return( FP_NORMAL );
}
