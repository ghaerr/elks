/*
 * pwent.c - This file is part of the libc-8086/pwd package for ELKS,
 * Copyright (C) 1995, 1996 Nat Friedman <ndf@linux.mit.edu>.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Sep 2023 Greg Haerr  - rewritten for speed, don't close passwd file
 */

#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <fcntl.h>
#include <paths.h>

static int __pwfd = -1;     /* file descriptor for passwd file */

void
setpwent(void)
{
    if (__pwfd < 0)
        __pwfd = open(_PATH_PASSWD, O_RDONLY);
    lseek(__pwfd, 0L, SEEK_SET);
}

void
endpwent(void)
{
    if (__pwfd >= 0) {
        close(__pwfd);
        __pwfd = -1;
    }
}

struct passwd *
getpwent(void)
{
    if (__pwfd < 0) {
        setpwent();
        if (__pwfd < 0)
            return NULL;
    }
    return __getpwent(__pwfd);
}
