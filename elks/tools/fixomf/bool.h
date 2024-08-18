/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2023 The Open Watcom Contributors. All Rights Reserved.
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


#if !defined( __cplusplus )
  #if !defined( __bool_true_false_are_defined )
    #if !defined( __STDC_VERSION__ ) || __STDC_VERSION__ < 199901L || defined( __WATCOMC__ ) && __WATCOMC__ < 1300
        /*
         * support for pre C99 compilers
         * fix for OW 1.9 bug in _Bool implementation
         */
        #define bool        unsigned char
        #define true        1
        #define false       0
        #define __bool_true_false_are_defined 1
    #else
        #include <stdbool.h>
    #endif
  #endif
  #if !defined( boolbit )
    #ifdef _MSC_VER
        #define boolbit     unsigned char
    #else
        #define boolbit     bool
    #endif
  #endif
#endif
