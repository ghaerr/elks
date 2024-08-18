/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2024 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Configuration for clib builds.
*
****************************************************************************/


#ifndef _VARIETY_H_INCLUDED
#define _VARIETY_H_INCLUDED

//
// Note: for the DLL versions of the runtime libraries, this file must be
//       included before any of the runtime header files.
//

#if !defined( __UNIX__ ) && !defined(__RDOS__) && !defined(__RDOSDEV__) && !defined( __NETWARE__ )
    #define CLIB_USE_MBCS_TRANSLATION
#endif
#if !defined( __UNIX__ ) && !defined( __RDOS__ ) && !defined( __RDOSDEV__ )
    #define CLIB_USE_OTHER_ENV
#endif
#if defined( __NT__ ) || defined( __RDOS__ ) || defined( __RDOSDEV__ )
    #define CLIB_UPDATE_OS_ENV
#endif

#ifndef __WATCOMC__
    // when building with other tools, only include clibext.h
    #include "clibext.h"
#else

#ifndef __COMDEF_H_INCLUDED
    #include <_comdef.h>
#endif

// specialized data reference macro
#define _HUGEDATA       _WCDATA

// memory model macros
#if defined(__SMALL__)
    #define __SMALL_DATA__
    #define __SMALL_CODE__
#elif defined(__FLAT__)
    #define __SMALL_DATA__
    #define __SMALL_CODE__
#elif defined(__MEDIUM__)
    #define __SMALL_DATA__
    #define __BIG_CODE__
#elif defined(__COMPACT__)
    #define __BIG_DATA__
    #define __SMALL_CODE__
#elif defined(__LARGE__)
    #define __BIG_DATA__
    #define __BIG_CODE__
#elif defined(__HUGE__)
    #define __BIG_DATA__
    #define __BIG_CODE__
#elif defined(__AXP__) || defined(__PPC__) || defined(__MIPS__)
    // these effectively use near data references
    #define __SMALL_DATA__
    #define __SMALL_CODE__
#else
    #error unable to configure memory model
#endif

// operating system and processor macros
#if defined(__GENERIC__)
    #if defined( _M_I86 )
        #define __REAL_MODE__
        #define __GENERIC_086__
    #elif defined( _M_IX86 )
        #define __PROTECT_MODE__
        #define __GENERIC_386__
    #else
        #error unrecognized processor for GENERIC
    #endif
#elif defined(__OS2__)
    #if defined( _M_I86 )
        #define __REAL_MODE__
        #define __OS2_286__
    #elif defined( _M_IX86 )
        #define __PROTECT_MODE__
        #define __OS2_386__
    #elif defined(__PPC__)
        #define __PROTECT_MODE__
        #define __OS2_PPC__
    #else
        #error unrecognized processor for OS2
    #endif
#elif defined(__NT__)
    #define __PROTECT_MODE__
    #if defined( _M_IX86 ) && !defined( _M_I86 )
        #define __NT_386__
    #elif defined(__AXP__)
        #define __NT_AXP__
    #elif defined(__PPC__)
        #define __NT_PPC__
    #elif defined(__MIPS__)
        #define __NT_MIPS__
    #else
        #error unrecognized processor for NT
    #endif
#elif defined(__WINDOWS__) || defined(__WINDOWS_386__)
    #define __PROTECT_MODE__
    #if defined( _M_I86 )
        #define __WINDOWS_286__
    #elif defined( _M_IX86 )
        #define __WINDOWS__
    #else
        #error unrecognized processor for WINDOWS
    #endif
#elif defined(__DOS__)
    #if defined( _M_I86 )
        #define __REAL_MODE__
        #define __DOS_086__
    #elif defined( _M_IX86 )
        #define __PROTECT_MODE__
        #define __DOS_386__
    #else
        #error unrecognized processor for DOS
    #endif
#elif defined(__QNX__)
    #define __PROTECT_MODE__
    #if defined( _M_I86 )
        #define __QNX_286__
    #elif defined( _M_IX86 )
        #define __QNX_386__
    #else
        #error unrecognized processor for QNX
    #endif
#elif defined(__LINUX__)
    #define __PROTECT_MODE__
    #if defined( _M_IX86 ) && !defined( _M_I86 )
        #define __LINUX_386__
    #elif defined(__PPC__)
        #define __LINUX_PPC__
    #elif defined(__MIPS__)
        #define __LINUX_MIPS__
    #else
        #error unrecognized processor for Linux
    #endif
#elif defined(__HAIKU__)
    #define __PROTECT_MODE__
    #if defined( _M_IX86 ) && !defined( _M_I86 )
        #define __HAIKU_386__
    #elif defined(__PPC__)
        #define __HAIKU_PPC__
    #else
        #error unrecognized processor for Haiku
    #endif
#elif defined(__NETWARE__)
    #define __PROTECT_MODE__
#elif defined(__RDOS__)
    #define __PROTECT_MODE__
#elif defined(__RDOSDEV__)
    #define __PROTECT_MODE__
#else
    #error unable to configure operating system and processor
#endif

// handle building dll's with appropriate linkage
#if !defined(__SW_BR) && (defined(__OS2__) && !defined(_M_I86) || defined(__NT__))
    #if defined(__MAKE_DLL_WRTLIB)
        #define _RTDLL
        #undef _WCRTLINK
        #undef _WCIRTLINK
        #undef _WCRTDATA
        #undef _WMRTLINK
        #undef _WMIRTLINK
        #undef _WMRTDATA
        #undef _WPRTLINK
        #undef _WPIRTLINK
        #undef _WPRTDATA
        #if defined(__NT__)
            #define _WCRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WCIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WCRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WMRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WMIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WMRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WPRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WPIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WPRTDATA  __declspec(dllexport) _WRTLDCONV
        #elif defined(__OS2__)
            #define _WCRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WCIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WCRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WMRTLINK  _WRTLFCONV
            #define _WMIRTLINK _WRTLFCONV
            #define _WMRTDATA  _WRTLDCONV
            #define _WPRTLINK  _WRTLFCONV
            #define _WPIRTLINK _WRTLFCONV
            #define _WPRTDATA  _WRTLDCONV
        #endif
    #elif defined(__MAKE_DLL_CLIB)
        #undef _WCRTLINK
        #undef _WCIRTLINK
        #undef _WCRTDATA
        #undef _WMRTLINK
        #undef _WMIRTLINK
        #undef _WMRTDATA
        #undef _WPRTLINK
        #undef _WPIRTLINK
        #undef _WPRTDATA
        #if defined(__NT__)
            #define _WCRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WCIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WCRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WMRTLINK  __declspec(dllimport) _WRTLFCONV
            #define _WMIRTLINK __declspec(dllimport) _WRTLFCONV
            #define _WMRTDATA  __declspec(dllimport) _WRTLDCONV
            #define _WPRTLINK  __declspec(dllimport) _WRTLFCONV
            #define _WPIRTLINK __declspec(dllimport) _WRTLFCONV
            #define _WPRTDATA  __declspec(dllimport) _WRTLDCONV
        #elif defined(__OS2__)
            #define _WCRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WCIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WCRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WMRTLINK  _WRTLFCONV
            #define _WMIRTLINK _WRTLFCONV
            #define _WMRTDATA  _WRTLDCONV
            #define _WPRTLINK  _WRTLFCONV
            #define _WPIRTLINK _WRTLFCONV
            #define _WPRTDATA  _WRTLDCONV
        #endif
    #elif defined(__MAKE_DLL_MATHLIB)
        #define _RTDLL
        #undef _WCRTLINK
        #undef _WCIRTLINK
        #undef _WCRTDATA
        #undef _WMRTLINK
        #undef _WMIRTLINK
        #undef _WMRTDATA
        #undef _WPRTLINK
        #undef _WPIRTLINK
        #undef _WPRTDATA
        #if defined(__NT__)
            #define _WCRTLINK  __declspec(dllimport) _WRTLFCONV
            #define _WCIRTLINK __declspec(dllimport) _WRTLFCONV
            #define _WCRTDATA  __declspec(dllimport) _WRTLDCONV
            #define _WMRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WMIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WMRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WPRTLINK  __declspec(dllimport) _WRTLFCONV
            #define _WPIRTLINK __declspec(dllimport) _WRTLFCONV
            #define _WPRTDATA  __declspec(dllimport) _WRTLDCONV
        #elif defined(__OS2__)
            #define _WCRTLINK  _WRTLFCONV
            #define _WCIRTLINK _WRTLFCONV
            #define _WCRTDATA  _WRTLDCONV
            #define _WMRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WMIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WMRTDATA  __declspec(dllexport) _WRTLDCONV
            #define _WPRTLINK  _WRTLFCONV
            #define _WPIRTLINK _WRTLFCONV
            #define _WPRTDATA  _WRTLDCONV
        #endif
    #elif defined(__MAKE_DLL_CPPLIB)
        #define _RTDLL
        #undef _WCRTLINK
        #undef _WCIRTLINK
        #undef _WCRTDATA
        #undef _WMRTLINK
        #undef _WMIRTLINK
        #undef _WMRTDATA
        #undef _WPRTLINK
        #undef _WPIRTLINK
        #undef _WPRTDATA
        #if defined(__NT__)
            #define _WCRTLINK  __declspec(dllimport) _WRTLFCONV
            #define _WCIRTLINK __declspec(dllimport) _WRTLFCONV
            #define _WCRTDATA  __declspec(dllimport) _WRTLDCONV
            #define _WMRTLINK  __declspec(dllimport) _WRTLFCONV
            #define _WMIRTLINK __declspec(dllimport) _WRTLFCONV
            #define _WMRTDATA  __declspec(dllimport) _WRTLDCONV
            #define _WPRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WPIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WPRTDATA  __declspec(dllexport) _WRTLDCONV
        #elif defined(__OS2__)
            #define _WCRTLINK  _WRTLFCONV
            #define _WCIRTLINK _WRTLFCONV
            #define _WCRTDATA  _WRTLDCONV
            #define _WMRTLINK  _WRTLFCONV
            #define _WMIRTLINK _WRTLFCONV
            #define _WMRTDATA  _WRTLDCONV
            #define _WPRTLINK  __declspec(dllexport) _WRTLFCONV
            #define _WPIRTLINK __declspec(dllexport) _WRTLFCONV
            #define _WPRTDATA  __declspec(dllexport) _WRTLDCONV
        #endif
    #endif
#endif

#define __ptr_check( p, a )
#define __null_check( p, a )
#define __stream_check( s, a )

#endif

#endif
