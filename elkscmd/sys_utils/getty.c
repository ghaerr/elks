/* 
 *  elkscmd/sysutils/getty.c
 *
 *  Copyright (C) 1998 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 *  Source for the /bin/getty command.
 *  
 *  usage: /bin/getty /dev/tty?? <speed>
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

/*
 * This is a small version of getty for use in the ELKS project.
 * It is not fully functional, and may not be the most efficient
 * implementation for larger systems. It minimises memory usage and
 * code size.
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define LOGIN	"/bin/login"
#define ISSUE	"/etc/issue"

char * nargv[3] = {NULL, NULL, NULL};
char buf[20];
int	n, fd;

void main(argc, argv)
int argc;
char ** argv;
{
	if ((fd = open(ISSUE, O_RDONLY)) > -1) {
		while(read(fd, &n, 1) != 0) write(STDOUT_FILENO, &n, 1);
	}
	
	for (;;) {
		write(STDOUT_FILENO, "login: ", 7);
		if (read(STDIN_FILENO, buf, sizeof(buf)) < 1) {
			exit(1);
		}
		if ((n = strlen(buf)-1) == 0) {
			continue;
		}
		if (buf[n] == '\n') {
		    buf[n] = 0;
		}
		
		nargv[0] = LOGIN;
		nargv[1] = buf;
		execv(nargv[0], nargv);
		exit(1);
	}
}
