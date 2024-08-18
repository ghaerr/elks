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
* Description:  Macros for single source wide/narrow character code.
*
****************************************************************************/


#ifndef _WIDECHAR_H_INCLUDED
#define _WIDECHAR_H_INCLUDED

/*** Define some handy macros ***/
#ifdef __WIDECHAR__
    #define UNICODE
    #define _UNICODE
    #define __UNICODE__

    #define DIR_TYPE            WDIR
    #define DIRENT_TYPE         struct _wdirent
    #define FINDT_TYPE          struct _wfind_t
    #define CHAR_TYPE           wchar_t
    #define UCHAR_TYPE          wchar_t
    #define INTCHAR_TYPE        wint_t
    #define UINTCHAR_TYPE       wint_t
    #define INT_WC_TYPE         wchar_t
    #define UINT_WC_TYPE        wchar_t
    #define VOID_WC_TYPE        wchar_t
    #define NULLCHAR            0
    #define STRING(a)           L##a
    #define _AToUni(p1,p2)      _atouni(p1,p2)
    #define __F_NAME(n1,n2)     n2
    #define INTCHAR_EOF         WEOF
    #define IS_ASCII_CHAR(c)    (((c) & ~0xff) == 0)
#else
    #define DIR_TYPE            DIR
    #define DIRENT_TYPE         struct dirent
    #define FINDT_TYPE          struct find_t
    #define CHAR_TYPE           char
    #define UCHAR_TYPE          unsigned char
    #define INTCHAR_TYPE        int
    #define UINTCHAR_TYPE       unsigned
    #define INT_WC_TYPE         int
    #define UINT_WC_TYPE        unsigned
    #define VOID_WC_TYPE        void
    #define NULLCHAR            '\0'
    #define STRING(a)           a
    #define _AToUni(p1,p2)      (p2)
    #define __F_NAME(n1,n2)     n1
    #define INTCHAR_EOF         EOF
    #define IS_ASCII_CHAR(c)    (1)
#endif

#define IS_ASCII_INT(c)         (c != (INTCHAR_TYPE)__F_NAME(EOF,WEOF) && ( c >= (INTCHAR_TYPE)-128 || c <= (INTCHAR_TYPE)255 ))
#define TO_ASCII(c)             ((unsigned char)(c))
#define CHARSIZE                (sizeof( CHAR_TYPE ))

#endif
