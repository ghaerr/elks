#define RELEASE		"tinyircd 0.1"
/* tinyircd 0.1
   Copyright (C) 1996 Nathan I. Laredo

   This program is modifiable/redistributable under the terms
   of the GNU General Public Licence.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   Send your comments and all your spare pocket change to
   laredo@gnu.ai.mit.edu (Nathan Laredo) or to PSC1, BOX 709,
   Lackland AFB, TX, 78236-5128
 */
#include <stdio.h>
#ifndef POSIX
#include <sgtty.h>
#define	USE_OLD_TTY
#include <sys/ioctl.h>
#if !defined(sun) && !defined(sequent) && !defined(__hpux) && \
	!defined(_AIX_)
#include <strings.h>
#define strchr index
#else
#include <string.h>
#endif
#else
#include <string.h>
#include <termios.h>
#endif
#include <pwd.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <utmp.h>

#define ischan(x) (*x == '#' || *x == '&' || *x == '+')

/* global structures */
struct channel_list {
    char *name;
    char *users;
    char *modes;
    char *banlist;
    char *moderators;
    time_t creation;
    struct channel_list *next;
};

struct user_list {
    char *nick;
    char *logname;
    char *loghost;
    char *realname;
    char *mode;
    time_t creation;
    int hopcount;
};
    
/*************************************************************/

int my_stricmp(str1, str2)
char *str1, *str2;
{
    int cmp;
    while (*str1 != 0 && str2 != 0) {
	if (isalpha(*str1) && isalpha(*str2)) {
	    cmp = *str1 ^ *str2;
	    if ((cmp != 32) && (cmp != 0))
		return (*str1 - *str2);
	} else {
	    if (*str1 != *str2)
		return (*str1 - *str2);
	}
	str1++;
	str2++;
    }
    return (*str1 - *str2);
}

int makeconnect(hostname)
char *hostname;
{
    struct sockaddr_in sa;
    struct hostent *hp;
    int s, t;
    if ((hp = gethostbyname(hostname)) == NULL)
	return -1;
    for (t = 0, s = -1; s < 0 && hp->h_addr_list[t] != NULL; t++) {
	bzero(&sa, sizeof(sa));
	bcopy(hp->h_addr_list[t], (char *) &sa.sin_addr, hp->h_length);
	sa.sin_family = hp->h_addrtype;
	sa.sin_port = htons((unsigned short) IRCPORT);
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s > 0)
	    if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		close(s);
		s = -1;
	    } else {
		fcntl(s, F_SETFL, O_NDELAY);
		my_tcp = s;
		sprintf(lineout, "NICK :%s\n", IRCNAME);
		sendline();
		sprintf(lineout, "USER %s * * :%s\n", IRCLOGIN, IRCGECOS);
		sendline();
	    }
    }
    return s;
}

main(argc, argv)
int argc;
char **argv;
{
    int i = 0;
    printf("%s Copyright (C) 1991-1996 Nathan Laredo\n\
This is free software with ABSOLUTELY NO WARRANTY.\n\
For details please see the file COPYING.\n", RELEASE);
    if (!(tmp = (char *) getenv("IRCSERVER")))
	strcpy(hostname, DEFAULTSERVER);
    else {
	while (*tmp && *tmp != ':')
	    hostname[i++] = *(tmp++);
	hostname[i] = '\0';
	if (*tmp == ':')
	    IRCPORT = (unsigned short) atoi(++tmp);
    }
    if (argc > 1) {
	for (i = 1; i < argc; i++)
	    if (argv[i][0] == '-') {
		if (argv[i][1] == 'd')
		    dumb = 1;
		else {
		    fprintf(stderr, "usage: %s %s\n", argv[0],
			    "[nick] [server] [port] [-dumb]");
		    exit(1);
		}
	    } else if (strchr(argv[i], '.'))
		strcpy(hostname, argv[i]);
	    else if (atoi(argv[i]) > 255)
		IRCPORT = atoi(argv[i]);
	    else
		strncpy(IRCNAME, argv[i], sizeof(IRCNAME));
    }
    if ((my_tty = open("/dev/tty", O_RDWR, 0)) == -1)
	my_tty = fileno(stdin);
    IRCGECOS[i = 63] = 0;
    if (!getpeername(my_tty, IRCGECOS, &i)) { /* inetd */
	strcpy(IRCNAME, IRCGECOS);
	strcpy(IRCLOGIN, "fromident");
	setenv("TERM", "vt102", 1);
    } else {
	userinfo = getpwuid(getuid());
	tmp = (char *) getenv("IRCNICK");
	if (tmp == NULL)
	    strncpy(IRCNAME, userinfo->pw_name, sizeof(IRCNAME));
	else
	    strncpy(IRCNAME, tmp, sizeof(IRCNAME));
	strcpy(IRCLOGIN, userinfo->pw_name);
	setutent();
	strcpy(ut.ut_line, strrchr(ttyname(0), '/') + 1);
	if ((utmp = getutline(&ut)) == NULL || !(utmp->ut_addr) ||
	    *((char *) utmp->ut_host) == ':' /* X connection */ )
	    tmp = userinfo->pw_gecos;
	else {
	    struct hostent *h;
	    struct in_addr a;
	    a.s_addr = utmp->ut_addr;
	    if (!(h = gethostbyaddr((char *) &a.s_addr,
				    sizeof(a.s_addr), AF_INET)))
		tmp = (char *) inet_ntoa(a);
	    else
		tmp = (char *) h->h_name;
	}
	strcpy(IRCGECOS, tmp);
	endutent();
    }
    fprintf(stderr, "*** User is %s\n", IRCGECOS);
    printf("*** trying port %d of %s\n\n", IRCPORT, hostname);
    if (makeconnect(hostname) < 0) {
	fprintf(stderr, "*** %s refused connection, aborting\n", hostname);
	exit(0);
    }
    idletimer = time(NULL);
    ptr = termcap;
    if ((term = (char *) getenv("TERM")) == NULL) {
	fprintf(stderr, "tinyirc: TERM not set\n");
	exit(1);
    }
    if (tgetent(bp, term) < 1) {
	fprintf(stderr, "tinyirc: no termcap entry for %s\n", term);
	exit(1);
    }
    if ((CO = tgetnum("co") - 2) < 20)
	CO = 78;
    if ((LI = tgetnum("li")) == -1)
	LI = 24;
    if (!dumb) {
#define tgs(x) ((char *) tgetstr(x, &ptr))
	if ((CM = tgs("cm")) == NULL)
	    CM = tgs("CM");
	if ((SO = tgs("so")) == NULL)
	    SO = "";
	if ((SE = tgs("se")) == NULL)
	    SE = "";
	if (!CM || !(CS = tgs("cs")) ||
	    !(CE = tgs("ce"))) {
	    printf("tinyirc: sorry, no termcap cm,cs,ce: dumb mode set\n");
	    dumb = 1;
	}
	if (!dumb) {
	    DC = tgs("dc");
	    savetty();
	    raw();
#ifdef CURSES
	    nonl();
	    noecho();
#endif
	}
    }
    redraw();
    signal(SIGINT, cleanup);
    signal(SIGHUP, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGSEGV, cleanup);
    signal(SIGTTIN, stopin);
    for (i = 0; i < HISTLEN; i++)
	hist[i] = (char *) calloc(512, sizeof(char));
    linein = hist[hline = 0];
    while (sok) {
	FD_ZERO(&readfs);
	FD_SET(my_tcp, &readfs);
	if (!noinput)
	    FD_SET(my_tty, &readfs);
	if (!dumb) {
	    timeout.tv_sec = 61;
	    timeout.tv_usec = 0;
	}
	if (select(FD_SETSIZE, &readfs, NULL, NULL, (dumb ? NULL : &timeout))) {
	    if (FD_ISSET(my_tty, &readfs))
		userinput();
	    if (FD_ISSET(my_tcp, &readfs))
		sok = serverinput();
	    if (!wasdate)
		updatestatus();
	} else
	    updatestatus();
	if (!sok && reconnect) {
	    close(my_tcp);	/* dead socket */
	    printf("*** trying port %d of %s\n\n", IRCPORT, hostname);
	    if (makeconnect(hostname) < 0) {
		fprintf(stderr, "*** %s refused connection\n", hostname);
		exit(0);
	    }
	    sok++;
	}
	if (!dumb)
	    tputs_x(tgoto(CM, curx % CO, LI - 1));
	fflush(stdout);
    }
    if (!dumb) {
	tputs_x(tgoto(CS, -1, -1));
	tputs_x(tgoto(CM, 0, LI - 1));
#ifdef CURSES
	echo();
	nl();
#endif
	resetty();
    }
    exit(0);
}
/* EOF */
