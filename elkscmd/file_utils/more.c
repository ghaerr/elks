/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

#include "futils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

int main(int argc, char **argv)
{
	FILE	*fp;
	int	fd, cin;
	char	*name;
	char	ch;
	int	line;
	int	col;
	char	buf[80];

	cin = 0;
	do {
		if(argc >= 2) {
		name = *(++argv);

		fd = open(name, O_RDONLY);
		if (fd == -1) {
			perror(name);
			exit(1);
		}

		write(STDOUT_FILENO,"<< ",3);
		write(STDOUT_FILENO,name,strlen(name));
		write(STDOUT_FILENO," >>\n",4);
		} else {
			fd = 0;
			cin = open("/dev/tty1", "r");
			if (!cin)
				cin = fopen("/dev/console", "r");
			write(STDOUT_FILENO,"<< stdin >>\n",12);
		}
		line = 1;
		col = 0;

		while ((fd > -1) && ((read(fd, &ch, 1)) != 0)) {
			switch (ch) {
				case '\r':
					col = 0;
					break;

				case '\n':
					line++;
					col = 0;
					break;

				case '\t':
					col = ((col + 1) | 0x07) + 1;
					break;

				case '\b':
					if (col > 0)
						col--;
					break;

				default:
					col++;
			}

			putchar(ch);
			if (col >= 80) {
				col -= 80;
				line++;
			}

			if (line < 25)
				continue;

			if (col > 0)
				putchar('\n');

			write(STDOUT_FILENO,"--More--",8);
			fflush(stdout);

			if (read(cin, buf, sizeof(buf)) < 1) {
				perror("more: ");
				if (fd > -1)
					close(fd);
				exit(0);
			}

			ch = buf[0];
			if (ch == ':')
				ch = buf[1];

			switch (ch) {
				case 'N':
				case 'n':
					close(fd);
					fd = -1;
					break;

				case 'Q':
				case 'q':
					close(fd);
					exit(0);
			}

			col = 0;
			line = 1;
		}
		if (fd)
			close(fd);
	} while(--argc > 1);
	exit(0);
}
