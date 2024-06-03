/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  ELKS ioctl() system call.
*
****************************************************************************/


#include <stdarg.h>
#include <sys/ioctl.h>
#include "watcom/syselks.h"


int ioctl( int __fd, int __request, ... )
{
    char *      argp;
    va_list     args;
    syscall_res res;

    va_start( args, __request );
    argp = va_arg( args, char * ); /* get largest of (int) and (char *) data types */
    va_end( args );
    sys_setseg(argp); /* (sets DS to garbage when int passed in large/compact model) */
    res = sys_call3( SYS_ioctl, __fd, __request, (unsigned)argp );
    __syscall_return( int, res );
}
