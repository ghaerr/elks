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
* Description:  Base 10 logarithm routine.
*
****************************************************************************/


#include "variety.h"
#include "mathlib.h"
#include "ifprag.h"


#define log10_of_e    0.4342944819032518


_WMRTLINK float _IF_log10( float x )
/**********************************/
{
    return( _IF_dlog10( x ) );
}

_WMRTLINK double (log10)( double x )
/**********************************/
{
    return( _IF_dlog10( x ) );
}

_WMRTLINK double _IF_dlog10( double x )
/*************************************/
{
    if( x <= 0.0 ) {
        x = __log_matherr( x, FP_FUNC_LOG10 );
    } else {
        x =  log(x) * log10_of_e;
    }
    return( x );
}
