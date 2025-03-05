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
* Description: Provides functions that return not-a-number
*
****************************************************************************/

#include "variety.h"
#include <math.h>

_WMRTDATA extern const float    __f_posqnan;
_WMRTDATA extern const double   __d_posqnan;

_WMRTLINK float nanf(const char *ignored)
{
    /* unused parameters */ (void)ignored;

    return __f_posqnan;
}

_WMRTLINK double nan(const char *ignored)
{
    /* unused parameters */ (void)ignored;

    return __d_posqnan;
}

#if defined(_LONG_DOUBLE_) && defined(_M_IX86)
_WMRTDATA extern const float    __ld_posqnan;
#endif

#ifdef _LONG_DOUBLE_
_WMRTLINK long double nanl(const char *ignored)
#else
_WMRTLINK double nanl(const char *ignored)
#endif
{
#ifdef _LONG_DOUBLE_

    /* unused parameters */ (void)ignored;

#ifdef _M_IX86
    return __ld_posqnan;
#else
    return (long double)__d_posqnan;
#endif
#else
    return nan(ignored);
#endif
}
