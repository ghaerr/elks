/*
 *  Copyright 1997 Marco Nelissen
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#define PREFIX "/boot/system"
#define HAVE_ATEXIT
#define HAVE_FCNTL_H
#define HAVE_GETOPT_H
#define HAVE_LIMITS_H
#define HAVE_SYS_IOCTL_H
#define TIME_WITH_SYS_TIME 1
#define HAVE_TERMIOS_H
#define HAVE_SYS_PARAM_H
#define HAVE_STRING_H
#define HAVE_MEMORY_H
#define HAVE_MALLOC_H
#define HAVE_UTIME_H
#define HAVE_SYS_WAIT_H
#define HAVE_MEMCPY
#define HAVE_MEMSET
#define HAVE_STRERROR
#define HAVE_STRNCASECMP
#define HAVE_STRCASECMP
#undef HAVE_GETPASS
#define HAVE_STDLIB_H
#define HAVE_STRCHR
#define HAVE_STRRCHR
#define HAVE_UNISTD_H
#define RETSIGTYPE void
#define HAVE_STRDUP
#define HAVE_STRPBRK
#define HAVE_STRSPN
#define HAVE_STRTOUL
#define HAVE_STRCSPN
#define HAVE_RANDOM
#define random rand
#define HAVE_SRANDOM
#define srandom srand
#define INIT_NOOP
#include <signal.h>
#define SYSCONFDIR "/boot/system"
#define USE_RAWTERM
