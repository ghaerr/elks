/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-2002,2007,2009 Alain Knaff.
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
 *
 * Do filename expansion with the shell.
 */

#include "sysincludes.h"
#include "mtools.h"

#ifndef OS_mingw32msvc
ssize_t safePopenOut(const char **command, char *output, size_t len)
{
	int pipefd[2];
	pid_t pid;
	int status;
	ssize_t last;

	if(pipe(pipefd)) {
		return -2;
	}
	switch((pid=fork())){
		case -1:
			return -2;
		case 0: /* the son */
			close(pipefd[0]);
			destroy_privs();
			close(1);
			close(2); /* avoid nasty error messages on stderr */
			if(dup(pipefd[1]) < 0) {
				perror("Dup error");
				exit(1);
			}
			close(pipefd[1]);
			execvp(command[0], (char**)(command+1));
			exit(1);
		default:
			close(pipefd[1]);
			break;
	}
	last=read(pipefd[0], output, len);
	kill(pid,9);
	wait(&status);
	if(last<0) {
		return -1;
	}
	return last;
}
#endif


const char *expand(const char *input, char *ans)
{
#ifndef OS_mingw32msvc
	ssize_t last;
	char buf[256];
	const char *command[] = { "/bin/sh", "sh", "-c", 0, 0 };

	ans[EXPAND_BUF-1]='\0';

	if (input == NULL)
		return(NULL);
	if (*input == '\0')
		return("");
					/* any thing to expand? */
	if (!strpbrk(input, "$*(){}[]\\?`~")) {
		strncpy(ans, input, EXPAND_BUF-1);
		return(ans);
	}
					/* popen an echo */
#ifdef HAVE_SNPRINTF
	snprintf(buf, 255, "echo %s", input);
#else
	sprintf(buf, "echo %s", input);
#endif

	command[3]=buf;
	last=safePopenOut(command, ans, EXPAND_BUF-1);
	if(last<0) {
		perror("Pipe read error");
		exit(1);
	}
	if(last)
		ans[last-1] = '\0';
	else
		strncpy(ans, input, EXPAND_BUF-1);
	return ans;
#else
	strncpy(ans, input, EXPAND_BUF-1);
	ans[EXPAND_BUF-1]='\0';
	return ans;
#endif
}
