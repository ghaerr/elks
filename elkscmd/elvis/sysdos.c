/* sysdos.c  -- DOS version of system.c */

/* Author:
 *	Guntram Blohm
 *	Buchenstrasse 19
 *	7904 Erbach, West Germany
 *	Tel. ++49-7305-6997
 *	sorry - no regular network connection
 */


/* This file is derived from Steve Kirkendall's system.c.
 *
 * Entry points are:
 *	system(cmd)	- run a single shell command
 *	wildcard(names)	- expand wildcard characters in filanames
 *
 * This file is for use with DOS and TOS. For OS/2, slight modifications
 * might be sufficient. For UNIX, use system.c. For Amiga, completely
 * rewrite this stuff.
 *
 * Another system function, filter, is the same on DOS and UNIX and thus
 * can be found in the original system.c.
 */

#include "config.h"
#include "vi.h"
extern char **environ;


#if MSDOS
#include <process.h>
extern unsigned char _osmajor;
#endif
#if TOS
#include <osbind.h>
#endif


#if MSDOS || TOS
#include <string.h>

/*
 * Calling command is a bit nasty because of the undocumented yet sometimes
 * used feature to change the option char to something else than /.
 * Versions 2.x and 3.x support this, 4.x doesn't.
 *
 * For Atari, some shells define a shortcut entry which is faster than
 * shell -c. Also, Mark Williams uses a special ARGV environment variable
 * to pass more than 128 chars to a called command.
 * We try to support all of these features here.
 */

int system(cmd)
	const char	*cmd;
{
#if MSDOS
	char *cmdswitch="/c";
	if (_osmajor<4)
		cmdswitch[0]=switchar();
	return spawnle(P_WAIT, o_shell, o_shell, cmdswitch, cmd, (char *)0, environ);
#else
	long	ssp;
	int	(*shell)();
	char	line[130];
	char	env[4096], *ep=env;
	int	i;

/* does our shell have a shortcut, that we can use? */

	ssp = Super(0L);
	shell = *((int (**)())0x4F6);
	Super(ssp);
	if (shell)
		return (*shell)(cmd);

/* else we'll have to call a shell ... */

	for (i=0; environ[i] && strncmp(environ[i], "ARGV=", 5); i++)
	{	strcpy(ep, environ[i]);
		ep+=strlen(ep)+1;
	}
	if (environ[i])
	{
		strcpy(ep, environ[i]); ep+=strlen(ep)+1;
		strcpy(ep, o_shell); ep+=strlen(ep)+1;
		strcpy(ep, "-c"); ep+=3;
		strcpy(ep, cmd); ep+=strlen(ep)+1;
	}
	*ep='\0';
	strcpy(line+1, "-c ");
	strncat(line+1, cmd, 126);
	line[0]=strlen(line+1);
	return Pexec(0, o_shell, line, env);
#endif
}

/* This private function opens a pipe from a filter.  It is similar to the
 * system() function above, and to popen(cmd, "r").
 * sorry - i cant use cmdstate until rpclose, but get it from spawnle.
 */

static int cmdstate;
static char output[80];

int rpipe(cmd, in)
	char	*cmd;	/* the filter command to use */
	int	in;	/* the fd to use for stdin */
{
	int	fd, old0, old1, old2;

	/* create the file that will collect the filter's output */
	strcpy(output, o_directory);
	if ((fd=strlen(output)) && !strchr("/\\:", output[fd-1]))
		output[fd++]=SLASH;
	strcpy(output+fd, SCRATCHIN+3);
	mktemp(output);
	close(creat(output, 0666));
	if ((fd=open(output, O_RDWR))==-1)
	{
		unlink(output);
		return -1;
	}

	/* save and redirect stdin, stdout, and stderr */
	old0=dup(0);
	old1=dup(1);
	if (in)
	{
		dup2(in, 0);
		close(in);
	}
	dup2(fd, 1);

	/* call command */
	cmdstate=system(cmd);

	/* restore old std... */
	dup2(old0, 0); close(old0);
	dup2(old1, 1); close(old1);

	/* rewind command output */
	lseek(fd, 0L, 0);
	return fd;
}

/* This function closes the pipe opened by rpipe(), and returns 0 for success */
int rpclose(fd)
	int	fd;
{
	int	status;

	close(fd);
	unlink(output);
	return cmdstate;
}

#endif
