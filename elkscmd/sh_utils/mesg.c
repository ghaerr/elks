/*
 * mesg.c
 *
 * Copyright 2000 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

/*
 * This is a small version of mesg for use in the ELKS project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

void usage(char ** argv)
{
	fprintf(stderr, "Usage: %s [y|n]\n", argv[0]);
	exit(1);
}

int main(int argc, char ** argv)
{
	struct stat sbuf;
	struct group * grp;
	char * tname;
	int oth = 0;

	if (argc > 3) {
		usage(argv);
	}

	if (fstat(STDIN_FILENO, &sbuf) != 0) {
		perror("stdin");
		exit(1);
	}
	if (!S_ISCHR(sbuf.st_mode) || (tname = ttyname(STDIN_FILENO)) == NULL) {
		fprintf(stderr, "stdin not a tty\n");
		exit(1);
	}
	if (((grp = getgrgid(sbuf.st_gid)) == NULL) ||
		(strcmp("tty", grp->gr_name))) {
		oth = 1;
	}
	if (argc == 1) {
		if (oth ? sbuf.st_mode & S_IWOTH : sbuf.st_mode & S_IWGRP) {
			printf("y\n");
		} else {
			printf("n\n");
		}
	} else { /* argc == 2 */
		switch (argv[1][0]) {
			case 'y':
				if (oth) {
					sbuf.st_mode |= (S_IWOTH | S_IWGRP);
				} else {
					sbuf.st_mode |= S_IWGRP;
				}
				break;
			case 'n':
				if (oth) {
					sbuf.st_mode &= ~(S_IWOTH | S_IWGRP);
				} else {
					sbuf.st_mode &= ~S_IWGRP;
				}
				break;
			default:
				usage(argv);
				exit(1);
		}
		if (chmod(tname, sbuf.st_mode) != 0) {
			perror(argv[0]);
		}
	}
	exit(0);
}
