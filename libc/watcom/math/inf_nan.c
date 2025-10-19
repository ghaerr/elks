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
* Description:  IEEE-754 Infinity and NaN constants.
*
****************************************************************************/


#include "variety.h"
#include <stdint.h>


/* NB: We can't easily include float.h here as the following
 * definitions are *not* using the types declared in float.h.
 */

typedef union {
    uint32_t        dw;
    float           x;
} float_t;

typedef union {
    uint64_t        qw;
    double          x;
} double_t;

typedef union {
    struct {
        uint32_t        lo_dw;
        uint32_t        hi_dw;
        uint16_t        exp;
    } ld;
    long double         x;
} long_double_t;

_WMRTDATA const float_t     __f_infinity  = { 0x7f800000 };
_WMRTDATA const float_t     __f_posqnan   = { 0x7fc00000 };

_WMRTDATA const double_t    __d_infinity  = { 0x7ff0000000000000 };
_WMRTDATA const double_t    __d_posqnan   = { 0x7ff8000000000000 };

#ifdef _LONG_DOUBLE_
  #ifdef _M_IX86
    _WMRTDATA const long_double __ld_infinity = { 0x00000000, 0x80000000, 0x7fff };
    _WMRTDATA const long_double __ld_posqnan  = { 0x00000000, 0xc0000000, 0x7fff };
  #else
    #error unknown long double format
  #endif
#else
    /* long double is the same type as double */
  #ifdef __BIG_ENDIAN__
    #error big endian needs fixing
  #else
    _WMRTDATA const double_t    __ld_infinity = { 0x7ff0000000000000 };
    _WMRTDATA const double_t    __ld_posqnan  = { 0x7ff8000000000000 };
  #endif
#endif
