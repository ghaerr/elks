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
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include "config.h"

#define	PATHLEN		256	
#define	CMDLEN		128
#define	MAXARGS		256
#define	ALIASALLOC	20
#define	STDIN		0
#define	STDOUT		1
#define	MAXSOURCE	10
#define	CFGFILE		"sashrc"

#ifndef	isblank
#define	isblank(ch)	(((ch) == ' ') || ((ch) == '\t'))
#endif

#define	isquote(ch)	(((ch) == '"') || ((ch) == '\''))
#define	isdecimal(ch)	(((ch) >= '0') && ((ch) <= '9'))
#define	isoctal(ch)	(((ch) >= '0') && ((ch) <= '7'))


typedef	int	BOOL;

#define	FALSE	((BOOL) 0)
#define	TRUE	((BOOL) 1)


extern	void	do_alias(), do_cd(), do_exec(), do_exit(), do_prompt();
extern	void	do_source(), do_umask(), do_unalias(), do_help(), do_ln();
extern	void	do_cp(), do_mv(), do_rm(), do_chmod(), do_mkdir(), do_rmdir();
extern	void	do_mknod(), do_chown(), do_chgrp(), do_sync(), do_printenv();
extern	void	do_more(), do_cmp(), do_touch(), do_ls(), do_dd(), do_tar();
extern	void	do_mount(), do_umount(), do_setenv(), do_pwd(), do_echo();
extern	void	do_kill(), do_grep(), do_ed(), do_history();


extern	char	*buildname();
extern	char	*modestring(mode_t mode);
extern	char	*timestring(time_t t);
extern	BOOL	isadir();
extern	BOOL	copyfile();
extern	BOOL	match();
extern	BOOL	makestring();
extern	BOOL	makeargs();
extern	int	expandwildcards();
extern	int	namesort();
extern	char	*getchunk();
extern	void	freechunks();

extern	BOOL	intflag;

/* END CODE */
