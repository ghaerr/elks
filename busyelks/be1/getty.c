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

/* For those requiring a super-small getty, the following define cuts out
 * all of the extra functionality regarding the /etc/issue code sequences.
 */

/* #define SUPER_SMALL		/* Disable for super-small binary */

/* For development work, the following two defines are available. The
 * first causes general debugging lines to be displayed, and the latter
 * causes a detailed trace of program execution to be displayed.
 */

/* #define DEBUG   		/* Enable for testing only */
/* #define TRACE   		/* Enable for testing only */

#ifdef DEBUG
#define debug(fmt)		fprintf(stderr,fmt)
#define debug1(fmt,a)		fprintf(stderr,fmt,a)
#define debug2(fmt,a,b) 	fprintf(stderr,fmt,a,b)
#else
#define debug(fmt)
#define debug1(fmt,a)
#define debug2(fmt,a,b)
#endif

#ifdef TRACE
#define trace(fmt)		fprintf(stderr,fmt)
#define trace1(fmt,a)		fprintf(stderr,fmt,a)
#define trace2(fmt,a,b) 	fprintf(stderr,fmt,a,b)
#else
#define trace(fmt)
#define trace1(fmt,a)
#define trace2(fmt,a,b)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define LOGIN		"/bin/login"
#define HOSTFILE	"/etc/HOSTNAME"
#define ISSUE		"/etc/issue"

char	Buffer[64];

#ifndef SUPER_SMALL
char	Host[256], *Date = 0, *Time = 0;
#endif

char	*nargv[3] = {NULL, NULL, NULL};
int	ch, col = 0, fd;

#ifndef SUPER_SMALL

void host(void) {
    char *ptr;
    int fp = open(HOSTFILE,O_RDONLY), sz;

    if (fp != NULL) {
	sz = read( fp, Host, 255);
	if (sz >= 0)
	    Host[sz] = '\0';
	else
	    *Host = '\0';
	close(fp);
    }
    for (ptr = Host; isprint(*ptr); ptr++)
	/* Do nothing */;
    while (ptr[-1] == ' ')
	ptr--;
    *ptr = '\0';
    if (!*Host)
	strcpy( Host, "LocalHost" );
    debug1( "DEBUG: host() = <%s>\n", Host );
}

#endif

void put(int ch) {
    if (ch == '\n' || ch == '\f')
	col = 0;
    else
	col++;
#ifdef TRACE
    if (ch < 32)
	fprintf(stderr, "TRACE: put(%3d) => Col %u\n", ch, col);
    else
	fprintf(stderr, "TRACE: put('%c') => Col %u\n", ch, col);
#else
    write(1,&ch,1);
#endif
}

void state(char *s) {
    trace1("TRACE: state( \"%s\" )\n",s);
    write(1,s,strlen(s));
}

#ifndef SUPER_SMALL

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
    debug2( "DEBUG: when() = <%s> @ <%s>\n", Date, Time );
}

#endif

void getty_main(int argc, char **argv) {
    char *ptr;
    int n;

    debug1("DEBUG: main( %d, **argv )\n",argc);
    if (argc < 2 || argc > 3) {
	fprintf(stderr,
		"\nERROR:   Invalid number of arguments: %d\nCommand: %s",
		argc-1,*argv);
	for (n=1; n<argc; n++)
	    fprintf(stderr," \"%s\"",argv[n]);
	fprintf(stderr,"\nUsage:   %s device [baudrate]\n\n",*argv);
#ifndef INIT_BUG_WORKAROUND
	exit(3);
#endif
    }
    debug("\n\n\nDEBUG: Running...\n");
    fd = open(ISSUE, O_RDONLY);
    trace1("TRACE: /etc/issue status = %d\n",fd);
    if (fd < 0) {
	perror("ERROR");
    } else {
	trace1("TRACE: File exists: %s\n", ISSUE);
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
	    trace1("TRACE: Found '%c'\n", ch);
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
		    debug1("DEBUG: Found '\\%c'\n",ch);
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
		    debug1("DEBUG: Found '@%c'\n",ch);
		    switch(ch) {
			case '@':
			    put(ch);
			    break;
			case 'B':			/* Baud Rate */
			    if (argv > 2) {
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
			    if (argv > 1) {
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
    trace1("TRACE: Finished with %s\n", ISSUE);
    for (;;) {
	state("login: ");
	n=read(STDIN_FILENO,Buffer,sizeof(Buffer)-1);
	if (n < 1)
	    exit(1);
	Buffer[n] = '\0';
	while (n > 0)
	    if (Buffer[--n] < ' ')
		Buffer[n] = '\0';
	if (*Buffer) {
	    nargv[0] = LOGIN;
	    nargv[1] = Buffer;
	    execv(nargv[0], nargv);
	    debug("DEBUG: Execv failed.\n\n");
	    exit(2);
	}
    }
}
