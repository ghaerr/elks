/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Definitions for stand-alone shell for system maintainance for Linux.
 * Stripped down and inlined for ELKS file_utils
 */

#define	isdecimal(ch)	(((ch) >= '0') && ((ch) <= '9'))
#define isoctal(ch)     (((ch) >= '0') && ((ch) <= '7'))

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))
