/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  ELKS select() implementation.
*
****************************************************************************/


#include <sys/select.h>
#include "watcom/syselks.h"


int select( int __width, fd_set * __readfds, fd_set * __writefds, fd_set * __exceptfds, struct timeval * __timeout )
{
    if (__readfds) sys_setseg(__readfds);
    else if (__writefds) sys_setseg(__writefds);
    else if (__timeout) sys_setseg(__timeout);
    syscall_res res = sys_call5( SYS_select, __width, (unsigned)__readfds, (unsigned)__writefds, (unsigned)__exceptfds, (unsigned)__timeout );
    __syscall_return( int, res );
}
