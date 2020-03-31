/*
 * elkscmd/sysutils/getty.c
 *
 * Copyright (C) 1998 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 *
 * Source for the /bin/getty command.
 *
 * usage: /bin/getty /dev/tty?? <speed>
 *
 * This file may be distributed under the terms of the GNU General Public
 * License version 2 or, at your option, any later version.
 *
 **************************************************************************
 *
 * This is a small version of getty for use in the ELKS project. It is not
 * fully functional, and may not be the most efficient implementation for
 * larger systems. It minimises memory usage and code size.
 *
 * Support for \? and @? codes has been added in, supporting the following
 * codes:
 *
 *	\@ = @			@@ = @
 *	\\ = \			@B = Baud Rate.
 *	\0 = ^@ 		@D = Date in dd-mmm-yyy format.
 *	\b = ^H 		@H = System hostname.
 *	\f = ^L 		@L = Line identifier.
 *	\n = ^J 		@S = System Identifier.
 *	\r = ^M 		@T = 24 hour time in HH:MM:SS format.
 *	\s = Space		@U = Users connected.
 *	\t = 8-column tab	@V = Kernel version.
 *
 * Note that @U is not yet implemented, and @V returns a fixed string
 * from the compile-time kernel rather than querying the current kernel.
 * These are all works in progress.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <stdarg.h>
#include <errno.h>

#define DEBUG		0			/* set =1 for debug messages*/

#define LOGIN		"/bin/login"
#define HOSTFILE	"/etc/HOSTNAME"
#define ISSUE		"/etc/issue"
#define CONSOLE		"/dev/tty1"

/* For those requiring a super-small getty, the following define cuts out
 * all of the extra functionality regarding the /etc/issue code sequences.
 */
//#define SUPER_SMALL		/* Disable for super-small binary */

#if DEBUG
#define debug		consolemsg
#else
#define debug(...)
#endif

char *	progname;
char	Buffer[64];
int	ch, col = 0, fd;

void consolemsg(const char *str, ...)
{
	static int consolefd = -1;
	char buf[80];

	if (consolefd < 0)
		consolefd = open(CONSOLE, O_RDWR);

	va_list args;
	va_start(args, str);
	sprintf(buf, "%s: ", progname);
	write(consolefd, buf, strlen(buf));
	vsprintf(buf, str, args);
	write(consolefd, buf, strlen(buf));
	va_end(args);
}


#ifndef SUPER_SMALL
char	Host[256], *Date = 0, *Time = 0;

void host(void) {
    char *ptr;
    int fp = open(HOSTFILE,O_RDONLY), sz;

    if (fp) {
	sz = read( fp, Host, 255);
	if (sz >= 0)
	    Host[sz] = '\0';
	else
	    *Host = '\0';
	close(fp);
    }
    for (ptr = Host; isprint(*ptr); ptr++)
	continue;
    while (ptr >= &Host[1] && ptr[-1] == ' ')
	ptr--;
    *ptr = '\0';
    if (!*Host)
	strcpy( Host, "LocalHost" );
}

/*	Before  = "Sun Dec 25 12:34:56 7890"
 *	Columns = "0....:....1....:....2..."
 *	After   = "25-Dec-7890 12:34:56 "
 */

void when(void) {
    char *Result;
    time_t now;
    int n;

    if (!Date) {
	now = time(0);
	Result = ctime(&now);

	Result[0]  = Result[8];
	Result[1]  = Result[9];

	Result[3]  = Result[4];
	Result[4]  = Result[5];
	Result[5]  = Result[6];

	Result[7]  = Result[20];
	Result[8]  = Result[21];
	Result[9]  = Result[22];
	Result[10] = Result[23];

	Result[2]  = Result[6]	 = '-';

	for (n=20; n>12; n--)
	    Result[n] = Result[n-1];

	Result[11] = Result[20] = '\0';

	Date = Result;
	if (*Date < '1')
	    Date++;
	Time = Result + 12;
    }
}
#endif

static void put(unsigned char ch)
{
    col++;
    if (ch == '\r' || ch == '\n')
	col = 0;
    write(STDOUT_FILENO, &ch, 1);
}

static void state(char *s)
{
    write(STDOUT_FILENO, s, strlen(s));
}

static speed_t convert_baudrate(speed_t baudrate)
{
	switch (baudrate) {
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
#ifdef B230400
	case 230400: return B230400;
#endif
#ifdef B460800
	case 460800: return B460800;
#endif
#ifdef B500000
	case 500000: return B500000;
#endif
#ifdef B576000
	case 576000: return B576000;
#endif
#ifdef B921600
	case 921600: return B921600;
#endif
#ifdef B1000000
	case 1000000: return B1000000;
#endif
	}
	return 0;
}


int main(int argc, char **argv)
{
    char *ptr;
    int n;
    speed_t baud = 0;
    struct termios termios;

    progname = argv[0];
    signal(SIGTSTP, SIG_IGN);		/* ignore ^Z stop signal*/

    if (argc < 2 || argc > 3) {
	consolemsg("Usage: %s device [baudrate]\n", argv[0]);
	exit(3);
    }

    if (argc == 2) debug("'%s'\n", argv[1]);
    else if (argc == 3) {
	baud = atol(argv[2]);
	debug("'%s' %ld\n", argv[1], baud);
    }

    fd = open(ISSUE, O_RDONLY);
    if (fd >= 0) {
	put(13);
#ifdef SUPER_SMALL
	while ((n=read(fd,Buffer,sizeof(Buffer))) > 0)
	    write(1,Buffer,n);
#else
	when();
	host();
	*Buffer = '\0';
	while (read(fd,Buffer,1) > 0) {
	    ch = *Buffer;
	    if (ch == '\\' || ch == '@') {
		Buffer[1] = ch;
		read(fd,Buffer+1,1);
	    }
	    switch (ch) {
		case '\n':
		    put(' ');
		    put(ch);
		    break;
		case '\\':
		    ch = Buffer[1];
		    switch(ch) {
			case '0':			/* NUL */
			    ch = 0;
			case '\\':
			case '@':
			    put(ch);
			    break;
			case 'b':			/* BS Backspace */
			    put(8);
			    break;
			case 'f':			/* FF Formfeed */
			    put(12);
			    break;
			case 'n':			/* LF Linefeed */
			    put(10);
			    break;
			case 's':			/* SP Space */
			    put(32);
			    break;
			case 't':			/* HT Tab */
			    do {
				put(' ');
			    } while (col & 7);
			    break;
			case 'r':			/* CR Return */
			    ch=13;
			default:			/* Anything else */
			    put('\\');
			    put(ch);
			    break;
		    }
		    break;
		case '@':
		    ch = Buffer[1];
		    switch(ch) {
			case '@':
			    put(ch);
			    break;
			case 'B':			/* Baud Rate */
			    if (argc > 2) {
				state(argv[2]);
				state(" Baud");
			    } else
				state("Terminal");
			    break;
			case 'D':			/* Date */
			    state(Date);
			    break;
			case 'H':			/* Host */
			    state(Host);
			    break;
			case 'L':			/* Line used */
			    if (argc > 1) {
				ptr = rindex(argv[1],'/');
				if (ptr == NULL)
				    ptr = argv[1];
			    } else
				ptr = NULL;
			    if (ptr == NULL)
				ptr = "tty";
			    state(ptr);
			    break;
			case 'S':			/* System */
			    state("ELKS");
			    break;
			case 'T':			/* Time */
			    state(Time);
			    break;
#if 0
			case 'U':			/* Users */
			    state("1 user");
			    break;
#endif
#ifdef ELKS_VERSION
			case 'V':			/* Version */
			    state(ELKS_VERSION);
			    break;
#endif
			default:
			    put('@');
			    put(ch);
			    break;
		    }
		    break;
		default:
		    put(ch);
		    break;
	    }
	    *Buffer = '\0';
	}
#endif
	close(fd);
    }

    /* setup tty termios state*/
    baud = convert_baudrate(baud);
    if (tcgetattr(STDIN_FILENO, &termios) >= 0) {
        termios.c_lflag |= ISIG | ICANON | ECHO | ECHOE | ECHONL;
        termios.c_lflag &= ~(IEXTEN | ECHOK | NOFLSH);
        termios.c_iflag |= BRKINT | ICRNL;
        termios.c_iflag &= ~(IGNBRK | IGNPAR | PARMRK | INPCK | ISTRIP | INLCR | IGNCR
		| IXON | IXOFF | IXANY);
        termios.c_oflag |= OPOST | ONLCR;
        termios.c_oflag &= ~XTABS;
        if (baud)
            termios.c_cflag = baud;
        termios.c_cflag |= CS8 | CLOCAL | HUPCL;
        /*termios.c_cflag |= CRTSCTS;*/
        termios.c_cflag &= ~(PARENB | CREAD);
        termios.c_cc[VMIN] = 0;
        termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);
    }

    for (;;) {
	state("login: ");
	n=read(STDIN_FILENO,Buffer,sizeof(Buffer)-1);
	if (n < 1) {
	    debug("read fail errno %d\n", errno);
	    if (errno != -EINTR)
		exit(1);
	    continue;
	}
	Buffer[n] = '\0';
	while (n > 0)
	    if (Buffer[--n] < ' ')
		Buffer[n] = '\0';
	if (*Buffer) {
	    char *nargv[3];

	    nargv[0] = LOGIN;
	    nargv[1] = Buffer;
	    nargv[2] = NULL;
	    execv(nargv[0], nargv);
	    debug("execv fail\n");
	    exit(2);
	}
    }
}
