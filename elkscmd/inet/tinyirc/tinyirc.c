#define COMMANDCHAR	'/'
#define ASCIIHEXCHAR	'@'
#define HEXASCIICHAR	'#'
#undef USE_ANSICOLOR
/* each line of hist adds 512 bytes to resident size */
#define HISTLEN		8
#ifdef AUTOJOIN
#define RELEASE		"TinyIRC 1.1 LinuxConv Edition"
#else
#define RELEASE		"TinyIRC 1.1"
#endif
/* most bytes to try to read from server at one time */
#define IB_SIZE		4096
/* TinyIRC 1.1
   Copyright (C) 1991-1996 Nathan I. Laredo

   This program is modifiable/redistributable under the terms
   of the GNU General Public Licence.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   Send your comments and all your spare pocket change to
   laredo@gnu.ai.mit.edu (Nathan Laredo) or to PSC1, BOX 709,
   Lackland AFB, TX, 78236-5128
 */
/* $Id$

   Modified for ELKS by Claudio Matsuoka <claudio@conectiva.com>

   $Log$
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

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
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <utmp.h>
struct dlist {
    char name[64];
    char mode[64];
    struct dlist *next;
};

#define ischan(x) (*x == 0x23 || *x == '&' || *x == '+')
#define OBJ obj->name
#define raise_sig(x) kill(getpid(), x)
struct dlist *obj = NULL, *olist = NULL, *newobj;
unsigned short IRCPORT = DEFAULTPORT;
int my_tcp, sok = 1, my_tty, hline, dumb = 0, CO, LI, column;
char *tmp, *linein, *CM, *CS, *CE, *SO, *SE, *DC, *ptr, *term, *fromhost,
*TOK[20], IRCNAME[32], IRCLOGIN[64], IRCGECOS[64], inputbuf[512], ib[IB_SIZE],
 serverdata[512], ch, *bp[1024], lineout[512], *hist[HISTLEN], termcap[1024];
int cursd = 0, curli = 0, curx = 0, noinput = 0, reconnect = 1;
fd_set readfs;
struct timeval timeout;
struct tm *timenow;
static time_t idletimer, datenow, wasdate, tmptime;
struct passwd *userinfo;
#ifdef  CURSES
#include <curses.h>
#else
#ifdef	POSIX
struct termios _tty;
tcflag_t _res_oflg, _res_lflg;
#define raw() (_tty.c_lflag &= ~(ICANON | ECHO | ISIG), \
	_tty.c_iflag &= ~ICRNL, \
	_tty.c_oflag &= ~ONLCR, tcsetattr(my_tty, TCSANOW, &_tty))
#define savetty() ((void) tcgetattr(my_tty, &_tty), \
	_res_oflg = _tty.c_oflag, _res_lflg = _tty.c_lflag)
#define resetty() (_tty.c_oflag = _res_oflg, _tty.c_lflag = _res_lflg,\
	(void) tcsetattr(my_tty, TCSADRAIN, &_tty))
#else
struct sgttyb _tty;
int _res_flg;
#define raw() (_tty.sg_flags |= RAW, _tty.sg_flags &= ~(ECHO | CRMOD), \
	ioctl(my_tty, TIOCSETP, &_tty))
#define savetty() ((void) ioctl(my_tty, TIOCGETP, &_tty), \
	_res_flg = _tty.sg_flags)
#define resetty() (_tty.sg_flags = _res_flg, \
	(void) ioctl(my_tty, TIOCSETP, &_tty))
#endif
#endif
int putchar_x(c)
int c;
{
    return putchar(c);
}
#define	tputs_x(s) (tputs(s,0,putchar_x))
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
struct dlist *additem(item, p)
char *item;
struct dlist *p;
{
    newobj = (struct dlist *) malloc(sizeof(struct dlist));
    strcpy(newobj->name, item);
    newobj->mode[0] = '\0';
    newobj->next = p;
    return newobj;
}
struct dlist *finditem(item, p)
char *item;
struct dlist *p;
{
    while (p != NULL)
	if (my_stricmp(item, p->name) == 0)
	    break;
	else
	    p = p->next;
    return p;
}
struct dlist *delitem(item, p)
char *item;
struct dlist *p;
{
    struct dlist *prev = NULL, *start = p;
    while (p != NULL)
	if (my_stricmp(item, p->name) == 0) {
	    newobj = p->next;
	    if (obj == p)
		obj = NULL;
	    free(p);
	    if (prev == NULL)
		return newobj;
	    else {
		prev->next = newobj;
		return start;
	    }
	} else {
	    prev = p;
	    p = p->next;
	}
    return start;
}

char encoded[512];
hexascii(s)
char *s;
{
    int ch, i, j, k, l;

    j = k = l = 0;
    for (i = 0; i < strlen(s) && j < 400; i++) {
	ch = toupper(s[i]);
	if (ch >= '0' && ch <= '9')
	    (l = (l << 4) | (ch - '0'), k++);
	else if (ch >= 'A' && ch <= 'F')
	    (l = (l << 4) | (ch - 'A' + 10), k++);
	if (k == 2)
	    (encoded[j++] = l, k = 0);
    }
    encoded[j] = '\0';
}
asciihex(s)
char *s;
{
    int i;

    *encoded = '\0';
    for (i = 0; i < strlen(s); i++)
	sprintf(&encoded[strlen(encoded)], "%02x", s[i]);
}
int sendline()
{
    if (write(my_tcp, lineout, strlen(lineout)) < 1)
	return 0;
    return 1;
}
void updatestatus()
{
    int n;
    if (!dumb) {
	if (60 < (datenow = time(NULL)) - wasdate) {
	    wasdate = datenow;
	    timenow = localtime(&datenow);
	    tputs_x(tgoto(CM, 0, LI - 2));
	    tputs_x(SO);
	    if (obj != NULL)
		n = printf("%02d:%02d %s %s (%s) %s", timenow->tm_hour,
			   timenow->tm_min, IRCNAME, OBJ,
			   obj->mode, RELEASE);
	    else
		n = printf("%02d:%02d %s * * %s", timenow->tm_hour,
			   timenow->tm_min, IRCNAME, RELEASE);
	    for (; n < CO; n++)
		putchar(' ');
	    tputs_x(SE);
	}
    }
}
static int nop()
{
    return 1;
}
static int doerror()
{
    column = printf("*** ERROR:");
    return 2;
}
static int doinvite()
{
    printf("*** %s (%s) invites you to join %s.",
	   TOK[0], fromhost, &TOK[3]);
    return 0;
}
static int dojoin()
{
    if (strcmp(TOK[0], IRCNAME) == 0) {
	obj = olist = additem(TOK[2], olist);
	sprintf(lineout, "MODE :%s\n", OBJ);
	sendline();
	printf("*** Now talking in %s", OBJ);
	wasdate = 0;
    } else
	printf("*** %s (%s) joined %s", TOK[0], fromhost, TOK[2]);
    return 0;
}
static int dokick()
{
    printf("*** %s was kicked from %s by %s (%s)",
	   TOK[3], TOK[2], TOK[0], TOK[4]);
    if (strcmp(TOK[3], IRCNAME) == 0) {
	olist = delitem(TOK[2], olist);
	if (obj == NULL)
	    obj = olist;
	if (obj != NULL)
	    printf("\n\r*** Now talking in %s", OBJ);
	wasdate = 0;
    }
    return 0;
}
static int dokill()
{
    printf("*** %s killed by %s: ", TOK[3], TOK[0]);
    if (strcmp(TOK[3], IRCNAME) == 0)
	reconnect = 0;		/* don't reconnect if killed */
    return 4;
}
static int domode()
{
    char *t = TOK[3], op = *TOK[3];
    printf("*** %s changed %s to:", TOK[0], TOK[2]);
    if ((newobj = finditem(TOK[2], olist)) != NULL) {
	while ((t = strpbrk(t, "-+psitnml")) != NULL) {
	    if (*t == '-' || *t == '+')
		op = *t;
	    else if (op == '-')
		for (tmp = strchr(newobj->mode, *t); *tmp != '\0'; tmp++)
		    *tmp = *(tmp + 1);
	    else
		strncat(newobj->mode, t, 1);
	    t++;
	}
	if (newobj == obj)
	    wasdate = 0;
    }
    return 3;
}
static int donick()
{
    if (strcmp(TOK[0], IRCNAME) == 0) {
	wasdate = 0;
	strcpy(IRCNAME, TOK[2]);
    }
    printf("*** %s is now known as %s", TOK[0], TOK[2]);
    return 0;
}
static int donotice()
{
    if (!ischan(TOK[2]))
	column = printf("-%s-", TOK[0]);
    else
	column = printf("-%s:%s-", TOK[0], TOK[2]);
    return 3;
}
static int dopart()
{
    printf("*** %s (%s) left %s", TOK[0], fromhost,
	   TOK[2]);
    if (strcmp(TOK[0], IRCNAME) == 0) {
	olist = delitem(TOK[2], olist);
	if (obj == NULL)
	    obj = olist;
	if (obj != NULL)
	    printf("\n\r*** Now talking in %s", OBJ);
	wasdate = 0;
    }
    return 0;
}
static int dopong()
{
    column = printf("*** Got PONG from %s:", TOK[0]);
    return 3;
}
static int doprivmsg()
{
#ifdef DO_CTCP
    if (strncmp(TOK[3], "\01PING", 5) == 0) {	/* lame ctcp ping hack */
	sprintf(lineout, "NOTICE %s :%s\n", TOK[0], TOK[3]);
	column = printf("*** CTCP PING from %s", TOK[0]);
	sendline();
	return 0;
    } else if (strncmp(TOK[3], "\01VERSION", 8) == 0) {		/* lame ctcp */
	sprintf(lineout, "NOTICE %s :\01VERSION " RELEASE " :*ix\01\n",
		TOK[0]);
	column = printf("*** CTCP VERSION from %s", TOK[0]);
	sendline();
	return 0;
    }
#endif
    if (!ischan(TOK[2])) {
#ifdef USE_ANSICOLOR
	printf("\E[1m");
#endif
	column = printf("*%s*", TOK[0]);
    } else if (obj != NULL && my_stricmp(OBJ, TOK[2]))
	column = printf("<%s:%s>", TOK[0], TOK[2]);
    else
	column = printf("<%s>", TOK[0]);
    return 3;
}
static int doquit()
{
    printf("*** %s (%s) Quit (%s)", TOK[0], fromhost, TOK[2]);
    return 0;
}
static int dosquit()
{
    return 1;
}
static int dotime()
{
    return 1;
}
static int dotopic()
{
    printf("*** %s set %s topic to \"%s\"", TOK[0], TOK[2],
	   TOK[3]);
    return 0;
}
int donumeric(num)
int num;
{
    switch (num) {
    case 352:
	column = printf("%-14s %-10s %-3s %s@%s :", TOK[3], TOK[7],
			TOK[8], TOK[4], TOK[5]);
	return 9;
    case 311:
	column = printf("*** %s is %s@%s", TOK[3], TOK[4], TOK[5]);
	return 6;
    case 324:
	if ((newobj = finditem(TOK[3], olist)) != NULL) {
	    strcpy(newobj->mode, TOK[4]);
	    wasdate = 0;
	}
	break;
    case 329:
	tmptime = atoi(TOK[4]);
	strcpy(lineout, ctime(&tmptime));
	tmp = strchr(lineout, '\n');
	if (tmp != NULL)
	    *tmp = '\0';
	column = printf("*** %s formed %s", TOK[3], lineout);
	return 0;
    case 333:
	tmptime = atoi(TOK[5]);
	strcpy(lineout, ctime(&tmptime));
	tmp = strchr(lineout, '\n');
	if (tmp != NULL)
	    *tmp = '\0';
	column = printf("*** %s topic set by %s on %s", TOK[3], TOK[4],
			lineout);
	return 0;
    case 317:
	tmptime = atoi(TOK[5]);
	strcpy(lineout, ctime(&tmptime));
	tmp = strchr(lineout, '\n');
	if (tmp != NULL)
	    *tmp = '\0';
	column = printf("*** %s idle %s second(s), on since %s",
			TOK[3], TOK[4], lineout);
	return 0;
    case 432:
    case 433:
	printf("*** You've chosen an invalid nick.  Choose again.");
	tmp = IRCNAME;
	if (!dumb) {
	    tputs_x(tgoto(CM, 0, LI - 1));
	    tputs_x(CE);
	    resetty();
	}
	printf("New Nick? ");
	while ((ch = getchar()) != '\n')
	    if (strlen(IRCNAME) < 9)
		*(tmp++) = ch;
	*tmp = '\0';
	if (!dumb) {
	    raw();
#ifdef CURSES
	    nonl();
	    noecho();
#endif
	    tputs_x(tgoto(CM, 0, LI - 1));
	    tputs_x(CE);
	}
	sprintf(lineout, "NICK :%s\n", IRCNAME);
	sendline();
#ifdef AUTOJOIN
	sprintf(lineout, AUTOJOIN);
	sendline();
#endif
	return wasdate = 0;
    default:
	break;
    }
    column = printf("%s", TOK[1]);
    return 3;
}
#define LISTSIZE 49
static char *clist[LISTSIZE] =
{"ADMIN", "AWAY", "CLOSE", "CONNECT", "DIE", "DNS", "ERROR", "HASH",
"HELP", "INFO", "INVITE", "ISON", "JOIN", "KICK", "KILL", "LINKS", "LIST",
 "LUSERS", "MODE", "MOTD", "MSG", "NAMES", "NICK", "NOTE", "NOTICE", "OPER",
 "PART", "PASS", "PING", "PONG", "PRIVMSG", "QUIT", "REHASH", "RESTART",
 "SERVER", "SQUIT", "STATS", "SUMMON", "TIME", "TOPIC", "TRACE", "USER",
 "USERHOST", "USERS", "VERSION", "WALLOPS", "WHO", "WHOIS", "WHOWAS"};
#define DO_JOIN 12
#define DO_MSG 20
#define DO_PART 26
#define DO_PRIVMSG 30
#define DO_QUIT 31
static int numargs[LISTSIZE] =
{
    15, 1, 15, 3, 15, 15, 15, 1, 15, 15, 15, 15, 15, 3, 2, 15, 15, 15,
    15, 15, 2, 1, 1, 2, 2, 15, 15, 1, 1, 1, 2, 1, 15, 15, 15, 2, 15,
    15, 15, 2, 15, 4, 15, 15, 15, 1, 15, 15, 15
};
static int (*docommand[LISTSIZE]) () =
{
    nop, nop, nop, nop, nop, nop, doerror, nop, nop, nop, doinvite,
	nop, dojoin, dokick, dokill, nop, nop, nop, domode, nop, nop, nop,
	donick, nop, donotice, nop, dopart, nop, nop, dopong, doprivmsg,
	doquit, nop, nop, nop, dosquit, nop, nop, dotime, dotopic, nop, nop,
	nop, nop, nop, nop, nop, nop, nop
};
int wordwrapout(p, count)
char *p;
int count;
{
    while (p != NULL) {
	if (tmp = strchr(p, ' '))
	    *(tmp++) = '\0';
	if (strlen(p) < CO - count)
	    count += printf(" %s", p);
	else
	    count = printf("\n\r   %s", p);
	p = tmp;
    }
    return count;
}
int parsedata()
{
    int i, found = 0;

    if (serverdata[0] == 'P') {
	sprintf(lineout, "PONG :%s\n", IRCNAME);
	return sendline();
    }
    if (!dumb)
	tputs_x(tgoto(CM, 0, LI - 3));
    TOK[i = 0] = serverdata;
    TOK[i]++;
    while (TOK[i] != NULL && i < 15)
	if (*TOK[i] == ':')
	    break;
	else {
	    if (tmp = strchr(TOK[i], ' ')) {
		TOK[++i] = &tmp[1];
		*tmp = '\0';
	    } else
		TOK[++i] = NULL;
	}
    if (TOK[i] != NULL && *TOK[i] == ':')
	TOK[i]++;
    TOK[++i] = NULL;
    if (tmp = strchr(TOK[0], '!')) {
	fromhost = &tmp[1];
	*tmp = '\0';
    } else
	fromhost = NULL;
    if (!dumb)
	putchar('\n');
    column = 0;
    if (i = atoi(TOK[1]))
	i = donumeric(i);
    else {
	for (i = 0; i < LISTSIZE && !found; i++)
	    found = (strcmp(clist[i], TOK[1]) == 0);
	if (found)
	    i = (*docommand[i - 1]) ();
	else
	    i = nop();
    }
    if (i) {
	if (*TOK[i] == ASCIIHEXCHAR && TOK[i + 1] == NULL) {
	    hexascii(&TOK[i][1]);
#ifdef USE_ANSICOLOR
	    printf("\E[1m");
#endif
	    wordwrapout(encoded);
#ifdef USE_ANSICOLOR
	    printf("\E[m");
#endif
	} else {
	    while (TOK[i])
		column = wordwrapout(TOK[i++], column);
#ifdef USE_ANSICOLOR
	    printf("\E[m");
#endif
	}
    }
    if (dumb)
	putchar('\n');
    if (strncmp(TOK[1], "Closing", 7) == 0)
	return (reconnect = 0);
    return 1;
}
int serverinput()
{
    int count, i;
    if ((count = read(my_tcp, ib, IB_SIZE)) < 1)
	return 0;
    for (i = 0; i < count; i++)
	if (ib[i] == '\n') {
	    serverdata[cursd] = '\0';
	    cursd = 0;
	    if (!parsedata())
		return 0;
	} else if (ib[i] != '\r')
	    serverdata[cursd++] = ib[i];
    return 1;
}
void parseinput()
{
    int i, j, outcol, c, found = 0;
    if (*linein == '\0')
	return;
    strcpy(inputbuf, linein);
    TOK[i = 0] = inputbuf;
    while (TOK[i] != NULL && i < 5)
	if (tmp = strchr(TOK[i], ' ')) {
	    TOK[++i] = &tmp[1];
	    *tmp = '\0';
	} else
	    TOK[++i] = NULL;
    TOK[++i] = NULL;
    if (!dumb) {
	tputs_x(tgoto(CM, 0, LI - 3));
	putchar('\n');
    }
    if (*TOK[0] == COMMANDCHAR) {
	TOK[0]++;
	for (i = 0; i < strlen(TOK[0]) && isalpha(TOK[0][i]); i++)
	    TOK[0][i] = toupper(TOK[0][i]);
	for (i = 0; i < LISTSIZE && !found; i++)
	    found = (strncmp(clist[i], TOK[0], strlen(TOK[0])) == 0);
	i--;
#ifdef AUTOJOIN
	if (!found || i == DO_JOIN || i == DO_PART) {
#else
	if (!found) {
#endif
	    printf("*** Invalid command");
	    return;
	}
#ifndef AUTOJOIN
	if (i == DO_JOIN)
	    if ((newobj = finditem(TOK[1], olist)) != NULL) {
		wasdate = 0;
		obj = newobj;
		printf("*** Now talking in %s", OBJ);
		return;
	    } else if (!ischan(TOK[1])) {
		obj = olist = additem(TOK[1], olist);
		printf("*** Now talking to %s", OBJ);
		wasdate = 0;
		return;
	    }
	if (i == DO_PART && !ischan(TOK[1]))
	    if ((newobj = finditem(TOK[1], olist)) != NULL) {
		wasdate = 0;
		olist = delitem(TOK[1], olist);
		if (obj == NULL)
		    obj = olist;
		printf("*** No longer talking to %s", TOK[1]);
		if (obj != NULL)
		    printf(", now %s", OBJ);
		wasdate = 0;
		return;
	    }
#endif
	if (i == DO_MSG)
	    i = DO_PRIVMSG;
	if (i == DO_PRIVMSG && (TOK[1] == NULL || TOK[2] == NULL)) {
	    printf("*** Unable to parse message");
	    return;
	}
	strcpy(lineout, clist[i]);
	if (i == DO_QUIT)
	    reconnect = 0;
	if (i == DO_QUIT && TOK[1] == NULL)
	    strcat(lineout, " :" RELEASE);
	j = 0;
	if (i != DO_PRIVMSG || TOK[1] == NULL)
	    outcol = printf("= %s", lineout);
	else if (ischan(TOK[1]))
	    outcol = printf(">%s>", TOK[1]);
	else {
#ifdef USE_ANSICOLOR
	    printf("\E[1m");
#endif
	    outcol = printf("-> *%s*", TOK[1]);
	}
	while (TOK[++j]) {
	    c = strlen(lineout);
	    sprintf(&lineout[c], "%s%s", ((j == numargs[i] &&
			      TOK[j + 1] != NULL) ? " :" : " "), TOK[j]);
	    if (j > 1 || i != DO_PRIVMSG)
		outcol = wordwrapout(TOK[j], outcol);
#ifdef USE_ANSICOLOR
	    printf("\E[m");
#endif
	}
	strcat(lineout, "\n");
    } else {
	if (obj == NULL) {
	    printf("*** Nowhere to send");
	    return;
	}
	if (*TOK[0] == ASCIIHEXCHAR) {
	    asciihex(&linein[1]);
	    strcpy(&linein[1], encoded);
	} else if (*TOK[0] == HEXASCIICHAR) {
	    /* display decoded line */
	    hexascii(&linein[1]);
	    outcol = wordwrapout(encoded);
	    return;
	}
	sprintf(lineout, "PRIVMSG %s :%s\n", OBJ, linein);
	outcol = printf("> %s", TOK[j = 0]);
	while (TOK[++j])
	    outcol = wordwrapout(TOK[j], outcol);
    }
    sendline();
    idletimer = time(NULL);
}
void mypchar(c)
char c;
{
    if (c >= ' ')
	putchar(c);
    else {
	tputs_x(SO);
	putchar(c + 64);
	tputs_x(SE);
    }
}
void ppart()
{
    int i, x = (curx / CO) * CO;
    tputs_x(tgoto(CM, 0, LI - 1));
    for (i = x; i < x + CO && i < curli; i++)
	mypchar(linein[i]);
    tputs_x(CE);
    tputs_x(tgoto(CM, curx % CO, LI - 1));
}
void histupdate()
{
    linein = hist[hline];
    curx = curli = strlen(linein);
    ppart();
}

void userinput()
{
    int i, z;
    if (dumb) {
	fgets(linein, 500, stdin);
	tmp = strchr(linein, '\n');
	if (tmp != NULL)
	    *tmp = '\0';
	parseinput();
	putchar('\n');
    } else {
	read(my_tty, &ch, 1);
	if (ch == '\177')
	    ch = '\10';
	switch (ch) {
	case '\3':
	    raise_sig(SIGINT);
	    break;
	case '\0':
	    if (curx >= CO) {
		curx = 0;
		ppart();
	    } else
		tputs_x(tgoto(CM, curx = 0, LI - 1));
	    break;
	case '\4':
	case '\10':
	    if (curx) {
		if (ch == '\4' && curx < curli)
		    curx++;
		if (curli == curx)
		    linein[(--curx)] = '\0';
		else
		    for (i = (--curx); i < curli; i++)
			linein[i] = linein[i + 1];
		curli--;
		if (DC != NULL && curx % CO != CO - 1) {
		    tputs_x(tgoto(CM, curx % CO, LI - 1));
		    tputs_x(DC);
		} else
		    ppart();
	    }
	    break;
	case '\2':
	    if (curx > 0)
		curx--;
	    if (curx % CO == CO - 1)
		ppart();
	    else
		tputs_x(tgoto(CM, curx % CO, LI - 1));
	    break;
	case '\5':
	    curx = curli;
	case '\14':
	    ppart();
	    break;
	case '\6':
	    if (curx < curli)
		curx++;
	    tputs_x(tgoto(CM, curx % CO, LI - 1));
	    break;
	case '\16':
	    if ((++hline) >= HISTLEN)
		hline = 0;
	    histupdate();
	    break;
	case '\20':
	    if ((--hline) < 0)
		hline = HISTLEN - 1;
	    histupdate();
	    break;
	case '\r':
	case '\n':
	    if (!curli)
		return;
	    tputs_x(tgoto(CM, 0, LI - 1));
	    tputs_x(CE);
	    parseinput();
	    if ((++hline) >= HISTLEN)
		hline = 0;
	    curx = curli = 0;
	    linein = hist[hline];
	    break;
	case '\27':
	    if (obj == NULL)
		break;
	    obj = obj->next;
	    if (obj == NULL)
		obj = olist;
	    wasdate = 0;
	    break;
	case '\32':
	    raise_sig(SIGTSTP);
	    break;
	default:
	    if (curli < 499) {
		if (curli == curx) {
		    linein[++curli] = '\0';
		    linein[curx++] = ch;
		    mypchar(ch);
		    tputs_x(CE);
		} else {
		    for (i = (++curli); i >= curx; i--)
			linein[i + 1] = linein[i];
		    linein[curx] = ch;
		    for (i = (curx % CO); i < CO &&
			 (z = (curx / CO) * CO + i) < curli; i++)
			mypchar(linein[z]);
		    tputs_x(CE);
		    curx++;
		}
	    }
	    break;
	}
    }
}
void cleanup(sig)
int sig;
{
    if (!dumb) {
	resetty();
	tputs_x(tgoto(CS, -1, -1));
	tputs_x(tgoto(CM, 0, LI - 1));
	fflush(stdout);
    }
#ifndef __hpux
#ifndef ELKS
    psignal(sig, "tinyirc");
#endif
#endif
    if (sig != SIGTSTP)
	exit(128 + sig);
    raise_sig(SIGSTOP);
}
void stopin()
{
    signal(SIGTTIN, stopin);
    noinput = 1;
}

void redraw()
{
    signal(SIGCONT, redraw);
    signal(SIGTSTP, cleanup);
    if (!dumb) {
	if (noinput) {
	    raw();
#ifdef CURSES
	    nonl();
	    noecho();
#endif
	}
	wasdate = 0;
	tputs_x(tgoto(CS, LI - 3, 0));
	updatestatus();
	tputs_x(tgoto(CM, LI - 3, 0));
    }
    noinput = 0;
}
int makeconnect(hostname)
char *hostname;
{
    struct sockaddr_in sa;
#ifndef ELKS
    struct hostent *hp;
    int s, t;
    if ((hp = gethostbyname(hostname)) == NULL)
	return -1;
    for (t = 0, s = -1; s < 0 && hp->h_addr_list[t] != NULL; t++) {
#else
    int s;
#endif
	bzero(&sa, sizeof(sa));
#ifndef ELKS
	bcopy(hp->h_addr_list[t], (char *) &sa.sin_addr, hp->h_length);
	sa.sin_family = hp->h_addrtype;
	sa.sin_port = htons((unsigned short) IRCPORT);
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
#else
	sa.sin_family = AF_INET;
	sa.sin_port = PORT_ANY;
	sa.sin_addr.s_addr = INADDR_ANY;
	s = socket(AF_INET, SOCK_STREAM, 0);
#endif
	if (s > 0) {
	    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        	perror("Bind failed");
		exit(1);
	    }
	}
	sa.sin_port = htons((unsigned short) IRCPORT);
	sa.sin_addr.s_addr = in_gethostbyname(hostname);
	if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	close(s);
	s = -1;
	} else {
	    fcntl(s, F_SETFL, O_NDELAY);
	    my_tcp = s;
	    sprintf(lineout, "USER %s * * :%s\n", IRCLOGIN, IRCGECOS);
	    sendline();
	    sprintf(lineout, "NICK :%s\n", IRCNAME);
	    sendline();
#ifdef AUTOJOIN
	    sprintf(lineout, AUTOJOIN);
	    sendline();
#endif
	    for (obj = olist; obj != NULL; obj = olist->next) {
		sprintf(lineout, "JOIN %s\n", OBJ);
		sendline();
	    }		/* error checking will be done later */
	}
#ifndef ELKS
    }
#endif
    return s;
}

main(argc, argv)
int argc;
char **argv;
{
    struct utmp ut, *utmp;
    char hostname[64];
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
#ifndef ELKS
    if (!getpeername(my_tty, IRCGECOS, &i)) { /* inetd */
	strcpy(IRCNAME, IRCGECOS);
	strcpy(IRCLOGIN, "fromident");
	setenv("TERM", "vt102", 1);
    } else
#endif
    {
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
#ifndef ELKS
	    if (!(h = gethostbyaddr((char *) &a.s_addr,
				    sizeof(a.s_addr), AF_INET)))
		tmp = (char *) inet_ntoa(a);
	    else
		tmp = (char *) h->h_name;
#else
	    tmp = (char *) hostname;
#endif
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
