/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)shell.h	5.4 (Berkeley) 4/12/91
 */

/*
 * The follow should be set to reflect the type of system you have:
 *	JOBS -> 1 if you have Berkeley job control, 0 otherwise.
 *	SYMLINKS -> 1 if your system includes symbolic links, 0 otherwise.
 *	DIRENT -> 1 if your system has the SVR3 directory(3X) routines.
 *	UDIR -> 1 if you want the shell to simulate the /u directory.
 *	TILDE -> 1 if you want the shell to expand ~logname.
 *	USEGETPW -> 1 if getpwnam() must be used to look up a name.
 *	ATTY -> 1 to include code for atty(1).
 *	SHORTNAMES -> 1 if your linker cannot handle long names.
 *	READLINE -> 1 if line editing by readline() should be enabled.
 *  LINENOISE -> 1 if line editing by linenoise() should be enabled
 *	define BSD if you are running 4.2 BSD or later.
 *	define SYSV if you are running under System V.
 *	define DEBUG=1 to compile in debugging (set global "debug" to turn on)
 *	define DEBUG=2 to compile in and turn on debugging.
 *
 * When debugging is on, debugging info will be written to $HOME/trace and
 * a quit signal will generate a core dump.
 */

#define JOBS	  0
#define SYMLINKS  0
#define DIRENT	  1
#define UDIR	  0
#define TILDE	  1
#define USEGETPW  0
#define ATTY	  0
#define READLINE  0
#define LINENOISE 1
/* #define BSD */
#define POSIX	  1
#define MAIL      0
#define DEBUG	  0

#ifdef __STDC__
typedef void *pointer;
#ifndef NULL
#define NULL (void *)0
#endif
#else /* not __STDC__ */
typedef char *pointer;
#ifndef NULL
#define NULL 0
#endif
#endif /*  not __STDC__ */
#define STATIC static	/* empty */
#define MKINIT	/* empty */

#include <sys/cdefs.h>

extern char nullstr[1];		/* null string */


#if DEBUG
#define TRACE(param)	trace param
#else
#define TRACE(param)
#endif
