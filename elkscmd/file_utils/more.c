/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "futils.h"

#define MORE_STRING  "\033[7m--More--\033[0m"
#define END_STRING   "\033[7m(END)\033[0m"
#define CLEAR_SCREEN "\033[H\033[2J"

#define WRITE(fd,str)   write(fd, str, strlen(str))

static int fd;
static int LINES = 25;
static int MAXLINES = 25;
static char cflag = 0;  /* -c flag: clear screen on start */

/* Use the DSR ESC [6n escape sequence to query the cursor position */
static int getCursorPosition(int ifd, int ofd, int *rows, int *cols)
{
	unsigned int i = 0;
	char buf[32];
	struct termios org, vmin;

	/* change to raw mode to wait 200ms instead of 1 character for DSR response*/
	if (tcgetattr(ifd, &org) < 0)
		return -1;
	vmin = org;
	vmin.c_iflag &= (IXON|IXOFF|IXANY|ISTRIP|IGNBRK);
	vmin.c_oflag &= ~OPOST;
	vmin.c_lflag &= ISIG;
	vmin.c_cc[VMIN] = 0; vmin.c_cc[VTIME] = 2; /* 0 bytes, 200ms timer */
	if (tcsetattr(ifd, TCSAFLUSH, &vmin) < 0)
		return -1;

	/* Send DSR (report cursor location) */
	write(ofd, "\x1b[6n", 4);

	/* Read the response: ESC [ rows ; cols R */
	while (i < sizeof(buf)-1) {
		if (read(ifd, buf+i, 1) != 1)
			break;
		if (buf[i++] == 'R')
			break;
	}
	buf[i] = '\0';

	/* reset to original mode*/
	tcsetattr(ifd, TCSAFLUSH, &org);

	/* Parse it. */
	if (buf[0] != 033 || buf[1] != '[')
		return -1;
	*rows = atoi(buf+2);
	char *p = buf+2;
	while (*p != ';')
		if (*p++ == '\0')
			return -1;
	if (*p == '\0')
		return -1;
	*cols = atoi(p+1);
	return 0;
}

/* Try to get the number of lines/columns from passed terminal file descriptors */
static int getWindowSize(int ifd, int ofd, int *rows, int *cols)
{
	int orig_row, orig_col;
	char seq[32];

	/* get initial cursor position so we can restore it later */
	if (getCursorPosition(ifd, ofd, &orig_row, &orig_col) < 0)
		return -1;

	/* goto right/bottom margin and get position */
	write(ofd,"\x1b[999C\x1b[999B",12);
	if (getCursorPosition(ifd, ofd, rows, cols) < 0)
		return -1;

	/* restore position */
	strcpy(seq, "\033[");
	strcat(seq, itoa(orig_row));
	strcat(seq, ";");
	strcat(seq, itoa(orig_col));
	strcat(seq, "H");
	write(ofd, seq, strlen(seq));
	return 0;
}

static int more_wait(int fout, char *msg)
{
	struct termios termios;
	char buf[80], ch;
	int cnt, ret = 0;

	write(fout, msg, strlen(msg));

	if (tcgetattr(1, &termios) >= 0) {
		struct termios termios2;
		tcgetattr(1, &termios2);
		termios2.c_lflag &= ~(ICANON|ECHO);
		termios2.c_cc[VMIN] = 1;
		tcsetattr(1, TCSAFLUSH, &termios2);
	}

	cnt = read(1, buf, sizeof(buf));
	LINES = MAXLINES;

	ch = buf[0];
	if (ch == ':') {
		write(fout, "\r          \r:", 13);
		if (cnt < 2) 
	           cnt = read(1, &buf[1], sizeof(buf)-1);
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
		    cnt = read(1, &buf[2], sizeof(buf)-2);
		if (buf[2] == 'G') 	/* rewind to beginning of file */
		    lseek(fd, 0, SEEK_SET);
		/* else just ignore */
		break;
	case '\n':
	case '\r':
		LINES = 2;
		break;
	case '2':
		LINES = 3;
		break;
	case '3':
		LINES = 4;
		break;
	}
	write(fout, "\r          \r", 12);
	tcsetattr(1, TCSAFLUSH, &termios);
	return ret;
}

static int cat_file(int ifd, int ofd)
{
	int n = 1;
	char mbuf[BUFSIZ];

	while (n > 0) {
		n = read(ifd, mbuf, sizeof(mbuf));
		if (n > 0)
			write(ofd, mbuf, n);
	}
	return n;
} 

int main(int argc, char **argv)
{
	int	multi, mw;
	int	line;
	int	col;
	char	*name, ch, next[80];
	char 	*divider = "\n::::::::::::::\n";

	if (isatty(2) && getWindowSize(2, 2, &line, &col) == 0)
		LINES = MAXLINES = line;
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
		if (cflag) WRITE(1, CLEAR_SCREEN);
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
#if 1
			if (col >= 80) {
				col -= 80;
				line++;
			}
#endif
			if (line < LINES)
				continue;

			if (col > 0)
				putchar('\n');

			if ((mw = more_wait(1, MORE_STRING)) > 0) {
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
		else more_wait(1, END_STRING);
		if (fd)
			close(fd);
	} while (--argc > 1);
	return 0;
}
