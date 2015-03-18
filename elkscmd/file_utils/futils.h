/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Definitions for stand-alone shell for system maintainance for Linux.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#define	PATHLEN		256	
#define	CMDLEN		512	
#define	MAXARGS		500	
#define	ALIASALLOC	20
#define	STDIN		0
#define	STDOUT		1
#define	MAXSOURCE	10

#define	isquote(ch)	(((ch) == '"') || ((ch) == '\''))
#define	isdecimal(ch)	(((ch) >= '0') && ((ch) <= '9'))
#define	isoctal(ch)	(((ch) >= '0') && ((ch) <= '7'))


typedef	int	BOOL;

#define	FALSE	((BOOL) 0)
#define	TRUE	((BOOL) 1)


/* END CODE */
