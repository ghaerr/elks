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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/*
        PDIV( n, d ) performs "n / d" properly,
        even on a Pentium with bad divide hardware.
*/
#ifndef _PDIV_H_INCLUDED
#define _PDIV_H_INCLUDED

#if defined(_M_IX86)
  extern unsigned _WCNEAR __chipbug;
  extern double __fdiv_chk( double n, double d );
  #pragma aux __fdiv_chk "*" __parm [__8087] __value [__8087]

  #if defined( __SW_FPC ) || (__WATCOMC__ >= 1000)
    #define PDIV( n, d )        ((n)/(d))
  #else
    #define PDIV( n, d )        ((__chipbug&1)?__fdiv_chk(n,d):(n)/(d))
  #endif
#else
  #define PDIV( n, d )  ((n)/(d))
#endif

#endif
