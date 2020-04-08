/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */

#include "futils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

int fd;
char mbuf[BUFSIZ];

int more_wait(int fin, int fout, char *msg)  {
	char buf[80], ch;

        write(fout, msg, strlen(msg));
                               
        if (read(fin, buf, sizeof(buf)) < 1) {
             perror("more: ");
             close(fd);
             return(-1);
        }       
        write(fout,"\r",1); /* move cursor to pos 0 */
        ch = buf[0];
        if (ch == ':')
              ch = buf[1];
                                       
        switch (ch) {
              case 'N':
              case 'n':
                   close(fd);
                   fd = -1;
                   return(1);
                               
              case 'Q':
              case 'q':
                   close(fd);
                   return(-1);
	}
	return(0);
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
	int	cin, multi, mw;
	char	*name, ch, next[80];
	int	line;
	int	col;
	char 	*divider = "\n::::::::::::::\n";

#if 0 /* temp fix */
	if ((cin = open("/dev/tty", O_RDONLY)) < 0) 
		fputs("Open /dev/tty failed, exiting\n", stderr); 
#endif /* 0 */
	cin = 2; /*temporary --- using stderr */
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

			if ((mw = more_wait(cin, 1, "--More--")) > 0) {
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
			if (more_wait(cin, 1, strcat(next, argv[1])) < 0)
				return 0;
		}
		if (fd)
			close(fd);
	} while (--argc > 1);
	close(cin);
	return 0;
}
