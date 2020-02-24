/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include "../sash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

void
more_main(argc, argv)
	char	**argv;
{
	FILE	*fp;
	int	fd;
	char	*name;
	char	ch;
	int	line;
	int	col;
	char	buf[80];

	while (argc-- > 1) {
		name = *(++argv);

		fd = open(name, O_RDONLY);
		if (fd == -1) {
			perror(name);
			exit(1);
		}

		write(STDOUT_FILENO,"<< ",3);
		write(STDOUT_FILENO,name,strlen(name));
		write(STDOUT_FILENO," >>\n",4);
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

			if (line < 24)
				continue;

			if (col > 0)
				putchar('\n');

			write(STDOUT_FILENO,"--More--",8);
			fflush(stdout);

			if ((read(0, buf, sizeof(buf)) < 0)) {
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
	}
	exit(0);
}
