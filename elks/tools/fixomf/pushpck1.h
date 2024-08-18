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
* Description:  Include this file to force 1-byte structure packing.
*
****************************************************************************/


#if defined( __WATCOMC__ )
    #pragma pack(__push,1)
#elif defined( _CFE ) || defined( __SUNPRO_C ) || defined( __sun )
    #if !defined( _NO_PRAGMA_PUSH_PACK )
        #define _NO_PRAGMA_PUSH_PACK
    #endif
    #pragma pack(1)
#elif defined( MAC )
    #if defined( __MWERKS__ )
        #pragma options align= packed
    #else
        #error "Need a push/pack for this Mac compiler"
    #endif
#elif defined( _MSC_VER )
    #pragma warning(disable:4103)
    #pragma pack(push,1)
#elif defined( __clang__ )
    #pragma pack(push,1)
#else
    #pragma pack(push,1)
#endif

#if defined( _NO_PRAGMA_PUSH_PACK )
    #if defined( _PUSH_PACK )
        #error Tried to push a pack at too great a depth
    #else
        #define _PUSH_PACK
    #endif
#endif
