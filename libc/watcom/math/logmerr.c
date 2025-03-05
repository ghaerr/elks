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
* Description:  Handles errors related to logarithmic functions,
*               passing actual errors on to the standard math error
*               handler.
*
****************************************************************************/


#include "variety.h"
#include <stddef.h>
#include "mathlib.h"


double __log_matherr( double x, unsigned char code )
/**************************************************/
{
    unsigned int    err_code;

    if( code != FP_FUNC_ACOSH && x == 0.0 ) {
        err_code = code | M_SING | V_NEG_HUGEVAL;
    } else if(fabs(x) == 0.0) {
        err_code = code | M_SING | V_NEG_HUGEVAL;
    } else {
        err_code = code | M_DOMAIN | V_NEG_HUGEVAL;
    }
    return( __math1err( err_code, &x ) );
}
