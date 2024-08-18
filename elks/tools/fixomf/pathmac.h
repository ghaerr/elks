/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2021 The Open Watcom Contributors. All Rights Reserved.
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



#ifdef _WIDECHAR_H_INCLUDED

#define EXT_SEP         STRING( '.' )
#ifdef __UNIX__
#define DIR_SEP         STRING( '/' )
#define DIR_SEP_STR     STRING( "/" )
#define IS_DIR_SEP(c)   (c == DIR_SEP)
#else
#define DRV_SEP         STRING( ':' )
#define DIR_SEP         STRING( '\\' )
#define DIR_SEP_STR     STRING( "\\" )
#define ALT_DIR_SEP     STRING( '/' )
#define ALT_DIR_SEP_STR STRING( "/" )
#define IS_DIR_SEP(c)   (c == ALT_DIR_SEP || c == DIR_SEP)
#define HAS_DRIVE(p)    (__F_NAME(isalpha,iswalpha)((UCHAR_TYPE)p[0]) && p[1]==DRV_SEP)
#define IS_ROOTDIR(p)   (HAS_DRIVE(p) && IS_DIR_SEP(p[2]) && p[3]==NULLCHAR)
#endif

#else

#define NULLCHAR        '\0'
#define EXT_SEP         '.'
#ifdef __UNIX__
#define DIR_SEP         '/'
#define DIR_SEP_STR     "/"
#define IS_DIR_SEP(c)   (c == DIR_SEP)
#else
#define DRV_SEP         ':'
#define DIR_SEP         '\\'
#define DIR_SEP_STR     "\\"
#define ALT_DIR_SEP     '/'
#define ALT_DIR_SEP_STR "/"
#define IS_DIR_SEP(c)   (c == ALT_DIR_SEP || c == DIR_SEP)
#define HAS_DRIVE(p)    (isalpha((unsigned char)p[0]) && p[1]==DRV_SEP)
#define IS_ROOTDIR(p)   (HAS_DRIVE(p) && IS_DIR_SEP(p[2]) && p[3]==NULLCHAR)
#endif

#endif
