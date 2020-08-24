/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

#include "futils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int fd;
char mbuf[BUFSIZ];

int more_wait(int fout, char *msg)
{
	struct termios termios;
	char buf[80], ch;
	int cnt, ret = 0;

	write(fout, msg, strlen(msg));

	if (tcgetattr(1, &termios) >= 0) {
		struct termios termios2;
		tcgetattr(1, &termios2);
		termios2.c_lflag &= ~ICANON;
		termios2.c_cc[VMIN] = 1;
		tcsetattr(1, TCSAFLUSH, &termios2);
	}

	cnt = read(1, buf, sizeof(buf));

	ch = buf[0];
	if (ch == ':') {
		write(fout, "\r          \r:", 13);
		if (cnt < 2) 
	           cnt = read(1, &buf[1], sizeof(buf-1));
		ch = buf[1];
	}
	switch (ch) {
	case 'N':
	case 'n':
		close(fd);
		fd = -1;
		ret = 1;
		break;
	case 'Q':
	case 'q':
		close(fd);
		ret = -1;
		break;
	case '1':
                if (cnt < 2) 
                   cnt = read(1, &buf[2], sizeof(buf-2));
		if (buf[2] == 'G') 	/* rewind to beginning of file */
		    (void) lseek(fd, 0, SEEK_SET);
		/* else just ignore */
	}
	write(fout, "\r          \r", 12);
	tcsetattr(1, TCSAFLUSH, &termios);
	return ret;
}

int cat_file(int m_in, int m_out) {
	int m_stat = 1;

	while (m_stat > 0) {
		m_stat = read(m_in, mbuf, BUFSIZ);
		if (m_stat > 0)
			write(m_out, mbuf, m_stat);
	}
	return (m_stat);
} 

int main(int argc, char **argv)
{
	int	multi, mw;
	int	line;
	int	col;
	char	*name, ch, next[80];
	char 	*divider = "\n::::::::::::::\n";

	multi = (argc >= 3); 		/* multiple input files */
	do {
		line = 1;
		col = 0;

		if (argc >= 2) {
			name = *(++argv);
			fd = open(name, O_RDONLY);
			if (fd == -1) {
				perror(name);
				return 1;
			}
			if (multi) {	/* if more than one file, print name */
				fputs(&divider[1], stdout);
				fputs(name, stdout);
				fputs(divider, stdout);
				fflush(stdout);
				line += 3;
			}
		} else 
			fd = 0;		/* use stdin */
		if (!isatty(1)) {	/* output is not terminal, just copy */
			if (cat_file(fd, 1) < 0) {
				perror("more :");
				return 1;
			}
			continue;
		}
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

			if ((mw = more_wait(1, "--More--")) > 0) {
				line = 1; /* user requested next file immediately */
				break;
			}
			if (mw < 0)
				return 0;
			col = 0;
			line = 1; 
		}
		if (multi && line > 1 && argc > 2) {
			strcpy(&next[0], "--Next file: "); 
			if (more_wait(1, strcat(next, argv[1])) < 0)
				return 0;
		}
		if (fd)
			close(fd);
	} while (--argc > 1);
	return 0;
}
