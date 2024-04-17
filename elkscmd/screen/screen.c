/*
 * Copyright (c) 1987,1988 Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute this
 * software, provided that no attempt is made to gain profit from it, the
 * author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is not
 * modified.
 *
 * This code was modified for use with ELKS and the bcc compiler (which only
 * supports K&R) by Riley Williams <rhw@MemAlpha.cx> with help from the Linux
 * ELKS development team on SourceForge.Net. It is based on the screen 2.0a
 * code as released on 19th October 1988.
 */

static char ScreenVersion[] = "screen 2.0a.2 (ELKS) 30-Apr-2020";

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termcap.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifdef ELKS
#include <linuxmt/un.h>
#else
#include <linux/un.h>
#define OPEN_MAX 1024
extern char **environ;
#endif

#ifdef UTMP
#include <utmp.h>
#endif

#ifdef SUNLOADAV
#include <sys/param.h>
#endif

#include "screen.h"

struct mode OldMode, NewMode;

#ifdef GETTTYENT
#include <ttyent.h>
#else
static struct ttyent {
    char *ty_name;
} *getttyent();
static char *tt, *ttnext;
static char ttys[] = "/etc/ttys";
#endif

#define MAXWIN     10
#define MSGWAIT     5

#define Ctrl(c) ((c)&037)

static int SigHandler(void);
static int DoWait(void);
static void CheckWindows(void);
static int ProcessInput(char *buf, int len);
static void SwitchWindow(int n);
static int SetCurrWindow(int n);
static int NextWindow(void);
static int PreviousWindow(void);
static int FreeWindow(struct win *wp);
static int ShowWindows(void);
static int MakeServerSocket(void);
static int MakeClientSocket(int err);
static int SendCreateMsg(int s, int ac, char **av, int aflag);
static int SendErrorMsg(char *fmt,...);
static void ReceiveMsg(int s);
static int ExecCreate(struct msg *mp);
static void ReadRc(char *fn);
static int Parse(char *fn, char *buf, char **args);
static char **SaveArgs(int argc, char **argv);
static int MakeNewEnv(void);
static int IsSymbol(char *e, char *s);
static char *Filename(char *s);
static int IsNum(char *s, int base);
static int RemoveUtmp(int slot);
static int SetUtmp(char *name);
static int InitUtmp(void);
static int MoreWindows(void);
static int MakeWindow(char *prog, char **args, int aflag, int StartAt, char *dir);
static int GetSockName(void);
static int Kill(int pid, int sig);
static void Attacher(void);
static int Attach(int how);
static void Detach(int suspend);
static int SetTTY(int fd, struct mode *mp);
static int GetTTY(int fd, struct mode *mp);
static int ShowInfo(void);
static void screen_execvpe(char *prog, char **args, char **env);
static void WriteFile(int dump);
static void KillWindow(struct win **pp);
static void Finit(int signum);
static int InitKeytab(void);
static int enableRawMode(int fd);
static void disableRawMode(int fd);
static void brktty(void);
static void freetty(void);

extern char *blank, Term[];
extern int rows, cols;
extern int ISO2022;
extern int status;
extern time_t TimeDisplayed;
extern char AnsiVersion[];
extern int flowctl;
extern char *MakeTermcap(int aflag);
extern char *getlogin(void);
static void AttacherFinit(int signum);
static void SigChld(int signum);
static void SigHup(int signum);
static char *MakeBellMsg(int n);
static char *GetTtyName(void);
static char PtyName[32], TtyName[32];
static char *ShellProg;
static char *ShellArgs[2];
static char inbuf[IOSIZE];
static int inlen;
static int ESCseen;
static int GotSignal;
static char DefaultShell[] = "/bin/sh";
static char DefaultPath[] = ":/bin:/usr/bin";
static int TtyMode = 0622;
static char SockPath[PATH_MAX];
static char SockDir[] = "screen";
static char *SockNamePtr, *SockName;
static int ServerSocket;
static char *NewEnv[MAXARGS];
static char Esc = Ctrl('a');
static char MetaEsc = 'a';
static char *home;
static int HasWindow;
#ifdef UTMP
static int utmp, utmpf;
static char UtmpName[] = "/etc/utmp";
static char *LoginName;
#endif
static char *BellString = "Bell in window %";
static int mflag, nflag, fflag, rflag;
static char HostName[MAXSTR];
static int Detached;
static int AttacherPid;             /* Non-Zero in child if we have an attacher */
static int DevTty;
#ifdef LOADAV
static char KmemName[] = "/dev/kmem";
#ifdef sequent
static char UnixName[] = "/dynix";
#else
static char UnixName[] = "/vmunix";
#endif
#ifdef alliant
static char AvenrunSym[] = "_Loadavg";
#else
static char AvenrunSym[] = "_avenrun";
#endif
static struct nlist nl[2];
static int avenrun, kmemf;
#ifdef SUNLOADAV
long loadav[3];
#else
#ifdef alliant
long loadav[4];
#else
double loadav[3];
#endif
#endif

#endif

static struct win *curr, *other;
static int CurrNum, OtherNum;
static struct win *wtab[MAXWIN];

#define KEY_IGNORE         0
#define KEY_HARDCOPY       1
#define KEY_SUSPEND        2
#define KEY_SHELL          3
#define KEY_NEXT           4
#define KEY_PREV           5
#define KEY_KILL           6
#define KEY_REDISPLAY      7
#define KEY_WINDOWS        8
#define KEY_VERSION        9
#define KEY_OTHER         10
#define KEY_0             11
#define KEY_1             12
#define KEY_2             13
#define KEY_3             14
#define KEY_4             15
#define KEY_5             16
#define KEY_6             17
#define KEY_7             18
#define KEY_8             19
#define KEY_9             20
#define KEY_XON           21
#define KEY_XOFF          22
#define KEY_INFO          23
#define KEY_TERMCAP       24
#define KEY_QUIT          25
#define KEY_DETACH        26
#define KEY_CREATE        27
#define KEY_HELP          28

struct key {
    int type;
    char **args;
} ktab[256];

char *KeyNames[] = {
    "hardcopy", "suspend", "shell", "next", "prev", "kill", "redisplay",
    "windows", "version", "other", "select0", "select1", "select2", "select3",
    "select4", "select5", "select6", "select7", "select8", "select9",
    "xon", "xoff", "info", "termcap", "quit", "detach", "help",
    0
};

static int SetMode(struct mode *op, struct mode *np);

static int rawmode = 0;
static struct termios orig_termios;

int
main(int ac, char **av)
{
    int n, len;
    struct win **pp, *p;
    char *ap;
    int s;
    fd_set r_readfd, w_writefd, e_errfd;
    int aflag = 0;
    int lflag = 0;
    struct timeval tv;
    time_t now;
    char buf[IOSIZE], *myname = (ac == 0) ? "screen" : av[0];
    char rc[PATH_MAX];
    struct stat st;

    while (ac > 0) {
        ap = *++av;
        if (--ac > 0 && *ap == '-') {
            switch (ap[1]) {
            case 'c':           /* Compatibility with older versions. */
                break;
            case 'a':
                aflag = 1;
                break;
            case 'l':
                lflag = 1;
                break;
            case 'm':
                mflag = 1;
                break;
            case 'n':
                nflag = 1;
                break;
            case 'f':
                fflag = 1;
                break;
            case 'r':
                rflag = 1;
                if (ap[2]) {
                    SockName = ap + 2;
                    if (ac != 1)
                        goto help;
                } else if (--ac == 1) {
                    SockName = *++av;
                } else if (ac != 0)
                    goto help;
                break;
            case 'e':
                if (ap[2]) {
                    ap += 2;
                } else {
                    if (--ac == 0)
                        goto help;
                    ap = *++av;
                }
                if (strlen(ap) != 2)
                    Msg(0, "Two characters are required with -e option.");
                Esc = ap[0] - 0x40;     /* a control character is difficult
                                         * to enter with an editor */
                MetaEsc = ap[1];
                break;
            default:
        help:
                exit_with_usage(myname);
            }
        } else
            break;
    }
    if (nflag && fflag)
        Msg(0, "-f and -n are conflicting options.");

    if ((ShellProg = getenv("SHELL")) == 0)
        ShellProg = DefaultShell;
    ShellArgs[0] = ShellProg;
    if (ac == 0) {
        ac = 1;
        av = ShellArgs;
    }
    if ((home = getenv("HOME")) == 0)
        Msg(0, "$HOME is undefined.");

    sprintf(SockPath, "%s/%s", home, SockDir);  /* SockDir = "screen" */
    if (stat(SockPath, &st) == -1) {
        if (errno == ENOENT) {
            if (mkdir(SockPath, 0700) == -1)
                Msg(errno, "Cannot make directory %s", SockPath);
            chown(SockPath, getuid(), getgid());
        } else {
            Msg(errno, "Cannot get status of %s", SockPath);
        }
    } else {
        if ((st.st_mode & S_IFMT) != S_IFDIR)
            Msg(0, "%s is not a directory.", SockPath);
        if ((st.st_mode & 0777) != 0700)
            Msg(0, "Directory %s must have mode 700.", SockPath);
        if (st.st_uid != getuid())
            Msg(0, "You are not the owner of %s.", SockPath);
    }

    gethostname(HostName, MAXSTR);
    if ((ap = strchr(HostName, '.')))
        *ap = '\0';
    strcat(SockPath, "/");
    SockNamePtr = SockPath + strlen(SockPath);
#if 1 //def BSDJOBS
    if ((DevTty = open("/dev/tty", O_RDWR | O_NDELAY)) == -1)
        Msg(errno, "/dev/tty");
#endif
    if (rflag) {
        Attach(MSG_ATTACH);
        Attacher();
        /* NOTREACHED */
    }

    if (GetSockName()) {
        s = MakeClientSocket(1);
        SendCreateMsg(s, ac, av, aflag);
        close(s);
        exit(0);
    }

    if (rflag) {
        switch (fork()) {
        case -1:
            Msg(errno, "fork");
            /* NOTREACHED */
        case 0:
            break;
        default:
            Attacher();
            /* NOTREACHED */
        }
    }
    AttacherPid = getppid();
    ServerSocket = s = MakeServerSocket();
    InitTerm();
    if (enableRawMode(STDIN_FILENO) == -1)
        return 0;
    if (fflag)
        flowctl = 1;
    else if (nflag)
        flowctl = 0;

    MakeNewEnv();
    GetTTY(0, &OldMode);
    InitUtmp();
    if (lflag) {
        Msg(0, "ShellProg: %s, SockPath: %s, HostName: %s, SockName:%s", ShellProg, SockPath, HostName, SockName);
    }
#ifdef LOADAV
    InitKmem();
#endif
    signal(SIGHUP, SigHup);
    signal(SIGINT, Finit);
    signal(SIGQUIT, Finit);
    signal(SIGTERM, Finit);
#ifdef BSDJOBS
    signal(SIGTTIN, SIG_IGN);
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
#endif
    InitKeytab();
    if ((n = MakeWindow(*av, av, aflag, 0, (char *)0)) == -1) {
        SetTTY(0, &OldMode);
        FinitTerm();
        disableRawMode(STDIN_FILENO);
        Kill(AttacherPid, SIGHUP);
        Msg(0, "MakeWindow failed");
        exit(1);
    }
    sprintf(rc, "%.*s/.screenrc", PATH_MAX - 11, home); /* TODO better limit for PATH_MAX */
    ReadRc(rc);                 /* need to allocate memory for win0 first */
    SetCurrWindow(n);
    HasWindow = 1;
    SetMode(&OldMode, &NewMode);
    SetTTY(0, &NewMode);
    signal(SIGCHLD, SigChld);
    tv.tv_usec = 0;
    while (1) {
        if (status) {
            time(&now);
            if (now - TimeDisplayed < MSGWAIT) {
                tv.tv_sec = MSGWAIT - (now - TimeDisplayed);
            } else
                RemoveStatus(curr);
        }

        /*
         * check for I/O on all available I/O descriptors
         */
        FD_ZERO(&r_readfd);
        FD_ZERO(&w_writefd);
        FD_ZERO(&e_errfd);

        if (inlen)
            FD_SET(curr->ptyfd, &w_writefd);
        else
            FD_SET(0, &r_readfd);
        for (pp = wtab; pp < wtab + MAXWIN; ++pp) {
            if (!(p = *pp))
                continue;
            if ((*pp)->active && status)
                continue;
            if ((*pp)->outlen > 0)
                continue;
            FD_SET((*pp)->ptyfd, &r_readfd);
        }
        FD_SET(s, &r_readfd);

        fflush(stdout);
        if (GotSignal && !status) {
            SigHandler();
            continue;
        }

        if ((select(32, &r_readfd, &w_writefd, &e_errfd, (status) ? &tv : (struct timeval *)0)) < 0) {
            //debug1("Bad select - errno %d\n", errno);
#if 0
            if (errno != EINTR) {
                HasWindow = 0;
                Msg(errno, "select");
                Finit();
            } else {
                errno = 0;
                if ((!GotSignal || status) && !InterruptPlease)
                    continue;
            }
#endif
        }

        if (GotSignal && !status) {
            SigHandler();
            continue;
        }

        if (FD_ISSET(s, &r_readfd)) {
            RemoveStatus(curr);
            ReceiveMsg(s);
        }

        if (FD_ISSET(0, &r_readfd)) {
            RemoveStatus(curr);
            if (ESCseen) {
                inbuf[0] = Esc;
                inlen = read(0, inbuf + 1, IOSIZE - 1) + 1;
                ESCseen = 0;
            } else {
                inlen = read(0, inbuf, IOSIZE);
            }
            if (inlen > 0)
                inlen = ProcessInput(inbuf, inlen);
            if (inlen > 0)
                continue;
        }

        if (GotSignal && !status) {
            SigHandler();
            continue;
        }

        if (FD_ISSET(curr->ptyfd, &w_writefd) && inlen > 0) {

            if ((len = write(curr->ptyfd, inbuf, inlen)) > 0) {
                inlen -= len;
                bcopy(inbuf + len, inbuf, inlen);
            }
        }

        if (GotSignal && !status) {
            SigHandler();
            continue;
        }

        for (pp = wtab; pp < wtab + MAXWIN; ++pp) {
            if (!(p = *pp))
                continue;
            if (p->outlen) {
                WriteString(p, p->outbuf, p->outlen);
            } else if (FD_ISSET(p->ptyfd, &r_readfd)) {

                if ((len = read(p->ptyfd, buf, IOSIZE)) == -1) {
                    if (errno == EWOULDBLOCK)
                        len = 0;
                }
                if (len > 0)
                    WriteString(p, buf, len);
            }
            if (p->bell) {
                p->bell = 0;
                Msg(0, MakeBellMsg(pp - wtab));
            }
        }

        if (GotSignal && !status)
            SigHandler();
    }                           /* while (1) */
    /* NOTREACHED */
}                               /* main */

static int
SigHandler(void)
{
    while (GotSignal) {
        GotSignal = 0;
        DoWait();
    }
    return 0;
}

static void
SigChld(int signum)
{
    GotSignal = 1;
}

static void
SigHup(int signum)
{
    Detach(0);
}

static int
DoWait(void)
{
    int pid;
    struct win **pp;
    int wstat;
    /* removed stuff ELKS does not support */
    return 0;
}

static void
KillWindow(struct win **pp)
{
    if (*pp == curr)
        curr = 0;
    if (*pp == other)
        other = 0;
    FreeWindow(*pp);
    *pp = 0;
}

static void
CheckWindows(void)
{
    struct win **pp;

    /*
     * If the current window disappeared and the "other" window is still
     * there, switch to the "other" window, else switch to the window with
     * the lowest index. If there current window is still there, but the
     * "other" window vanished, "SetCurrWindow" is called in order to assign
     * a new value to "other". If no window is alive at all, exit.
     */
    if (!curr && other) {
        SwitchWindow(OtherNum);
        return;
    }
    if (curr && !other) {
        SetCurrWindow(CurrNum);
        return;
    }
    for (pp = wtab; pp < wtab + MAXWIN; ++pp) {
        if (*pp) {
            if (!curr)
                SwitchWindow(pp - wtab);
            return;
        }
    }
    Finit(0);
}

static void
Finit(int signum)
{
    struct win *p, **pp;

    for (pp = wtab; pp < wtab + MAXWIN; ++pp) {
        if ((p = *pp))
            FreeWindow(p);
    }
    SetTTY(0, &OldMode);
    FinitTerm();
    disableRawMode(STDIN_FILENO);
    printf("\r\n[screen is terminating]\n");
    Kill(AttacherPid, SIGHUP);
    exit(0);
}

static int
InitKeytab(void)
{
    int i;

    ktab['h'].type = ktab[Ctrl('h')].type = KEY_HARDCOPY;
    ktab['z'].type = ktab[Ctrl('z')].type = KEY_SUSPEND;
    ktab['c'].type = ktab[Ctrl('c')].type = KEY_SHELL;
    ktab[' '].type = ktab[Ctrl(' ')].type =
        ktab['n'].type = ktab[Ctrl('n')].type = KEY_NEXT;
    ktab['-'].type = ktab['p'].type = ktab[Ctrl('p')].type = KEY_PREV;
    ktab['k'].type = ktab[Ctrl('k')].type = KEY_KILL;
    ktab['l'].type = ktab[Ctrl('l')].type = KEY_REDISPLAY;
    ktab['w'].type = ktab[Ctrl('w')].type = KEY_WINDOWS;
    ktab['v'].type = ktab[Ctrl('v')].type = KEY_VERSION;
    ktab['q'].type = ktab[Ctrl('q')].type = KEY_XON;
    ktab['s'].type = ktab[Ctrl('s')].type = KEY_XOFF;
    ktab['t'].type = ktab[Ctrl('t')].type = KEY_INFO;
    ktab['.'].type = KEY_TERMCAP;
    ktab[Ctrl('\\')].type = ktab['K'].type = KEY_QUIT;
    ktab['d'].type = ktab[Ctrl('d')].type = KEY_DETACH;
    ktab['?'].type = KEY_HELP;
    ktab[Esc].type = KEY_OTHER;
    for (i = 0; i <= 9; i++)
        ktab[i + '0'].type = KEY_0 + i;
    return 0;
}

static int
ProcessInput(char *buf, int len)
{
    int n, k;
    char *s, *p;
    struct win **pp;

    for (s = p = buf; len > 0; len--, s++) {
        if (*s == Esc) {
            if (len > 1) {
                len--;
                s++;
                k = ktab[*s].type;
                if (*s == MetaEsc) {
                    *p++ = Esc;
                } else if (k >= KEY_0 && k <= KEY_9) {
                    p = buf;
                    SwitchWindow(k - KEY_0);
                } else
                    switch (ktab[*s].type) {
                    case KEY_TERMCAP:
                        p = buf;
                        WriteFile(0);
                        break;
                    case KEY_HARDCOPY:
                        p = buf;
                        WriteFile(1);
                        break;
                    case KEY_SUSPEND:
                        p = buf;
                        /* Detach (1); */
                        Msg(0, "Detach disabled");
                        break;
                    case KEY_SHELL:
                        p = buf;
                        if ((n = MakeWindow(ShellProg, ShellArgs,
                                            0, 0, (char *)0)) != -1) {
                            SwitchWindow(n);
                            DoESC('c', 0);      /* clear screen */
                            ShowWindows();
                        }
                        break;
                    case KEY_NEXT:
                        p = buf;
                        if (MoreWindows())
                            SwitchWindow(NextWindow());
                        break;
                    case KEY_PREV:
                        p = buf;
                        if (MoreWindows())
                            SwitchWindow(PreviousWindow());
                        break;
                    case KEY_KILL:
                        p = buf;
                        FreeWindow(wtab[CurrNum]);
                        if (other == curr)
                            other = 0;
                        curr = wtab[CurrNum] = 0;
                        CheckWindows();
                        ShowWindows();
                        break;
                    case KEY_QUIT:
                        for (pp = wtab; pp < wtab + MAXWIN; ++pp)
                            if (*pp)
                                FreeWindow(*pp);
                        Finit(0);
                        /* NOTREACHED */
                    case KEY_DETACH:
                        p = buf;
                        /* Detach (0); */
                        Msg(0, "Detach disabled");
                        break;
                    case KEY_REDISPLAY:
                        p = buf;
                        Activate(wtab[CurrNum]);
                        break;
                    case KEY_WINDOWS:
                        p = buf;
                        ShowWindows();
                        break;
                    case KEY_VERSION:
                        p = buf;
                        Msg(0, "%s  %s", ScreenVersion, AnsiVersion);
                        break;
                    case KEY_HELP:
                        display_help();
                        break;
                    case KEY_INFO:
                        p = buf;
                        ShowInfo();
                        break;
                    case KEY_OTHER:
                        p = buf;
                        if (MoreWindows())
                            SwitchWindow(OtherNum);
                        break;
                    case KEY_XON:
                        *p++ = Ctrl('q');
                        break;
                    case KEY_XOFF:
                        *p++ = Ctrl('s');
                        break;
                    case KEY_CREATE:
                        p = buf;
                        if ((n = MakeWindow(ktab[*s].args[0], ktab[*s].args,
                                            0, 0, (char *)0)) != -1) {
                            SwitchWindow(n);
                        }
                        break;
                    }
            } else
                ESCseen = 1;
        } else
            *p++ = *s;
    }
    return p - buf;
}

static void
SwitchWindow(int n)
{
    if (!wtab[n])
        return;
    SetCurrWindow(n);
    Activate(wtab[n]);
}

static int
SetCurrWindow(int n)
{
    /*
     * If we come from another window, this window becomes the "other"
     * window:
     */
    if (curr) {
        curr->active = 0;
        other = curr;
        OtherNum = CurrNum;
    }
    CurrNum = n;
    curr = wtab[n];
    curr->active = 1;
    /*
     * If the "other" window is currently undefined (at program start or
     * because it has died), or if the "other" window is equal to the one
     * just selected, we try to find a new one:
     */
    if (other == 0 || other == curr) {
        OtherNum = NextWindow();
        other = wtab[OtherNum];
    }
    return 0;
}

static int
NextWindow(void)
{
    struct win **pp;

    for (pp = wtab + CurrNum + 1; pp != wtab + CurrNum; ++pp) {
        if (pp == wtab + MAXWIN)
            pp = wtab;
        if (*pp)
            break;
    }
    return pp - wtab;
}

static int
PreviousWindow(void)
{
    struct win **pp;

    for (pp = wtab + CurrNum - 1; pp != wtab + CurrNum; --pp) {
        if (pp < wtab)
            pp = wtab + MAXWIN - 1;
        if (*pp)
            break;
    }
    return pp - wtab;
}

static int
MoreWindows(void)
{
    struct win **pp;
    int n;

    for (n = 0, pp = wtab; pp < wtab + MAXWIN; ++pp)
        if (*pp)
            ++n;
    if (n <= 1)
        Msg(0, "No other window.");
    return n > 1;
}

static int
FreeWindow(struct win *wp)
{
    int i;

    RemoveUtmp(wp->slot);
    chmod(wp->tty, 0666);
    chown(wp->tty, 0, 0);
    close(wp->ptyfd);
    for (i = 0; i < rows; ++i) {
        free(wp->image[i]);
        free(wp->attr[i]);
        free(wp->font[i]);
    }
    free(wp->image);
    free(wp->attr);
    free(wp->font);
    free(wp);
    return 0;
}

int
MakeWindow(char *prog, char **args, int aflag, int StartAt, char *dir)
{
    struct win **pp, *p;
    char **cp;
    int n, f;
    int tf;
    //int mypid;
    char ebuf[10];

    pp = wtab + StartAt;
    do {
        if (*pp == 0)
            break;
        if (++pp == wtab + MAXWIN)
            pp = wtab;
    } while (pp != wtab + StartAt);
    if (*pp) {
        Msg(0, "No more windows.");
        return -1;
    }
    n = pp - wtab;
    sprintf(PtyName, "/dev/ptyp%d", n);
    f = open(PtyName, O_RDWR);
    sprintf(TtyName, "/dev/ttyp%d", n);
    if (f == -1) {
        Msg(0, "No more PTYs.");
        return -1;
    }
    fcntl(f, F_SETFL, O_NDELAY);
    if ((p = *pp = (struct win *)malloc(sizeof(struct win))) == 0) {

nomem:
        Msg(0, "Out of memory.");
        return -1;
    }

    /* allocate memory */
    if ((p->image = (char **)malloc(rows * sizeof(char *))) == 0)
        goto nomem;

    for (cp = p->image; cp < p->image + rows; ++cp) {
        if ((*cp = malloc(cols)) == 0)
            goto nomem;
        //bclear(*cp, cols);
        bzero(*cp, cols);
    }

    if ((p->attr = (char **)malloc(rows * sizeof(char *))) == 0)
        goto nomem;

    for (cp = p->attr; cp < p->attr + rows; ++cp) {
        if ((*cp = malloc(cols)) == 0)
            goto nomem;
        bzero(*cp, cols);
    }

    if ((p->font = (char **)malloc(rows * sizeof(char *))) == 0)
        goto nomem;

    if ((p->tabs = malloc(cols + 1)) == 0)      /* +1 because 0 <= x <= cols */
        goto nomem;

#if 0
    for (cp = p->font; cp < p->font + rows; ++cp) {
        if ((*cp = malloc(cols)) == 0)
            goto nomem;
        bzero(*cp, cols);
    }
#endif

    ResetScreen(p);
    p->aflag = aflag;
    p->active = 0;
    p->bell = 0;
    p->outlen = 0;
    p->ptyfd = f;
    strncpy(p->cmd, Filename(args[0]), MAXSTR - 1);
    p->cmd[MAXSTR - 1] = '\0';
    strncpy(p->tty, TtyName, MAXSTR - 1);
    chown(TtyName, getuid(), getgid());
    chmod(TtyName, TtyMode);
    p->slot = SetUtmp(TtyName);
    switch (p->wpid = fork()) {
    case -1:
        Msg(errno, "fork");
        free((char *)p);
        return -1;
    case 0:
        signal(SIGHUP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
#ifdef SIGTTOU
        signal(SIGTTOU, SIG_DFL);
#endif
        setuid(getuid());
        setgid(getgid());
        if (dir && chdir(dir) == -1) {
            SendErrorMsg("Cannot chdir to %s: %s", dir, strerror(errno));
            exit(1);
        }
        //mypid = getpid();
        //ioctl(DevTty, TIOCNOTTY, (char *)0);
        freetty();
        if ((tf = open(TtyName, O_RDWR)) == -1) {
            SendErrorMsg("Cannot open %s: %s", TtyName, strerror(errno));
            exit(1);
        }
        dup2(tf, 0);
        dup2(tf, 1);
        dup2(tf, 2);
        for (f = OPEN_MAX - 1; f > 2; f--)
            close(f);
        //ioctl(0, TIOCSPGRP, &mypid);
        //setpgrp(0, mypid);
        //not supported by ELKS - just dummy
        /* posix setsid() in brktty() from freetty() already made us leader */
            SetTTY(0, &OldMode);
        NewEnv[2] = MakeTermcap(aflag);
        sprintf(ebuf, "WINDOW=%d", n);
        NewEnv[3] = ebuf;
        screen_execvpe(prog, args, NewEnv);
        SendErrorMsg("Cannot exec %s: %s", prog, strerror(errno));
        exit(1);
    }
    return n;
}

static void
screen_execvpe(char *prog, char **args, char **env)
{
    char *path, *p;
    char buf[MAXLINE * 2];
    char *shargs[MAXARGS + 1];
    int i, eaccess = 0;

    if (prog[0] == '/')
        path = "";
    else if ((path = getenv("PATH")) == 0)
        path = DefaultPath;
    do {
        p = buf;
        while (*path && *path != ':')
            *p++ = *path++;
        if (p > buf)
            *p++ = '/';
        strcpy(p, prog);
        if (*path)
            ++path;
        execve(buf, args, env);
        switch (errno) {
        case ENOEXEC:
            shargs[0] = DefaultShell;
            shargs[1] = buf;
            for ((i = 1); (shargs[i + 1] = args[i]); ++i)
                ;
            execve(DefaultShell, shargs, env);
            return;
        case EACCES:
            eaccess = 1;
            break;
        case ENOMEM:
        case E2BIG:
        case ETXTBSY:
            return;
        }
    } while (*path);
    if (eaccess)
        errno = EACCES;
}

static void
WriteFile(int dump)
{
    /*
     * dump==0: create .termcap, dump==1: hardcopy
     */
    int i, j, k;
    char *p;
    FILE *f;
    char fn[PATH_MAX];
    int pid, s;

    if (dump)
        sprintf(fn, "hardcopy.%d", CurrNum);
    else
        sprintf(fn, "%s/%s/termcap", home, SockDir);
    switch (pid = fork()) {
    case -1:
        Msg(errno, "fork");
        return;
    case 0:
        setuid(getuid());
        setgid(getgid());
        if ((f = fopen(fn, "w")) == NULL)
            exit(1);
        if (dump) {
            for (i = 0; i < rows; ++i) {
                p = curr->image[i];
                for (k = cols - 1; k >= 0 && p[k] == ' '; --k);
                for (j = 0; j <= k; ++j)
                    putc(p[j], f);
                putc('\n', f);
            }
            Msg(0, "Text screen written to file \"%s\".\n\r", fn);
        } else {
            if ((p = strchr(MakeTermcap(curr->aflag), '='))) {
                fputs(++p, f);
                putc('\n', f);
            }
            Msg(0, "Termcap file \"%s\" written to disk.\n\r", fn);
        }
        fclose(f);
        exit(0);

    default:
        while ((i = wait(&s)) != pid)
            if (i == -1)
                return;
        if ((s >> 8) & 0377)
            Msg(0, "Cannot open \"%s\".", fn);
        else
            Msg(0, "%s written to \"%s\".", dump ? "Screen image" :
                "Termcap entry", fn);
    }
}

static int
ShowWindows(void)
{
    char buf[1024];
    char *s;
    struct win **pp, *p;

    for (s = buf, pp = wtab; pp < wtab + MAXWIN; ++pp) {
        if ((p = *pp) == 0)
            continue;
        if (s - buf + 5 + strlen(p->cmd) > cols - 1)
            break;
        if (s > buf) {
            *s++ = 'W';
            *s++ = 'i';
            *s++ = 'n';
            *s++ = ':';
        }
        *s++ = pp - wtab + '0';
        if (p == curr)
            *s++ = '*';         /* indicate current window */
        else if (p == other)
            *s++ = ' ';         /* blank for other */
        *s++ = '-';
        strcpy(s, p->cmd);
        s += strlen(s);
        strcpy(s, ", ");
        s += 2;
    }
    Msg(0, "Win:%s", buf);
    return 0;
}

static int
ShowInfo(void)
{
    char buf[1024], *p;
    struct win *wp = curr;
    int i;
    struct tm *tp;
    time_t now;

    time(&now);
    tp = localtime(&now);
    sprintf(buf, "%2d:%2.2d:%2.2d %s", tp->tm_hour, tp->tm_min, tp->tm_sec,
            HostName);
#ifdef LOADAV
    if (avenrun && GetAvenrun()) {
        p = buf + strlen(buf);
#ifdef SUNLOADAV
        sprintf(p, " %2.2f %2.2f %2.2f", (double)loadav[0] / FSCALE,
                (double)loadav[1] / FSCALE, (double)loadav[2] / FSCALE);
#else
#ifdef alliant
        sprintf(p, " %2.2f %2.2f %2.2f %2.2f", (double)loadav[0] / 100,
                (double)loadav[1] / 100, (double)loadav[2] / 100,
                (double)loadav[3] / 100);
#else
        sprintf(p, " %2.2f %2.2f %2.2f", loadav[0], loadav[1], loadav[2]);
#endif
#endif
    }
#endif
    p = buf + strlen(buf);
    sprintf(p, " (%d,%d) %cflow %cins %corg %cwrap %cpad", wp->y, wp->x,
            flowctl ? '+' : '-',
            wp->insert ? '+' : '-', wp->origin ? '+' : '-',
            wp->wrap ? '+' : '-', wp->keypad ? '+' : '-');
    if (ISO2022) {
        p = buf + strlen(buf);
        sprintf(p, " G%1d [", wp->LocalCharset);
        for (i = 0; i < 4; i++)
            p[i + 5] = wp->charsets[i] ? wp->charsets[i] : 'B';
        p[9] = ']';
        p[10] = '\0';
    }
    Msg(0, buf);
    return 0;
}

static int
SetTTY(int fd, struct mode *mp)
{
#ifdef BSD_TTY
    ioctl(fd, TIOCSETP, &mp->m_ttyb);
    ioctl(fd, TIOCSETC, &mp->m_tchars);
    ioctl(fd, TIOCSLTC, &mp->m_ltchars);
    ioctl(fd, TIOCLSET, &mp->m_lmode);
    ioctl(fd, TIOCSETD, &mp->m_ldisc);
#endif
    return 0;
}

static int
GetTTY(int fd, struct mode *mp)
{
#ifdef BSD_TTY
    ioctl(fd, TIOCGETP, &mp->m_ttyb);
    ioctl(fd, TIOCGETC, &mp->m_tchars);
    ioctl(fd, TIOCGLTC, &mp->m_ltchars);
    ioctl(fd, TIOCLGET, &mp->m_lmode);
    ioctl(fd, TIOCGETD, &mp->m_ldisc);
#endif
    return 0;
}

static int
SetMode(struct mode *op, struct mode *np)
{
    *np = *op;
#ifdef BSD_TTY
    np->m_ttyb.sg_flags &= ~(CRMOD | ECHO);
    np->m_ttyb.sg_flags |= CBREAK;
    np->m_tchars.t_intrc = -1;
    np->m_tchars.t_quitc = -1;
    if (!flowctl) {
        np->m_tchars.t_startc = -1;
        np->m_tchars.t_stopc = -1;
    }
    np->m_ltchars.t_suspc = -1;
    np->m_ltchars.t_dsuspc = -1;
    np->m_ltchars.t_flushc = -1;
    np->m_ltchars.t_lnextc = -1;
#endif
    return 0;
}

static char *
GetTtyName(void)
{
    char *p;
    int n;

    for (p = 0, n = 0; n <= 2 && !(p = ttyname(n)); n++)
        ;
    if (!p || *p == '\0')
        Msg(0, "screen must run on a tty.");
    return p;
}

static int
Attach(int how)
{
    int s, lasts = 0, found = 0;
    DIR *dirp;
    struct dirent *dp;
    struct msg m;
    char last[MAXNAMLEN + 1];
    if (SockName) {
        if ((lasts = MakeClientSocket(0)) == -1) {
            if (how == MSG_CONT)
                Msg(0, "This screen has already been continued from elsewhere.");
            else
                Msg(0, "There is no screen to be resumed from %s.", SockName);
        }
    } else {
        if ((dirp = opendir(SockPath)) == NULL)
            Msg(0, "Cannot open %s", SockPath);

        while ((dp = readdir(dirp)) != NULL) {
            SockName = dp->d_name;
            if (SockName[0] == '.')     /* . and .. */
                continue;
            if ((s = MakeClientSocket(0)) != -1) {
                if (found == 0) {
                    strcpy(last, SockName);
                    lasts = s;
                } else {
                    if (found == 1) {
                        printf("There are detached screens on:\n");
                        printf("   %s\n", last);
                        close(lasts);
                    }
                    printf("   %s\n", SockName);
                    close(s);
                }
                found++;
            }
        }
        if (found == 0)
            Msg(0, "There is no screen to be resumed.");
        if (found > 1)
            Msg(0, "Type \"screen -r ttyX.scr\" to resume one of them.");
        closedir(dirp);
        strcpy(SockNamePtr, last);
        SockName = SockNamePtr;
    }
    m.type = how;
    strcpy(m.m.attach.tty, GetTtyName());
    m.m.attach.apid = getpid();
    if (write(lasts, (char *)&m, sizeof(m)) != sizeof(m))
        Msg(errno, "write");
    return 0;
}

static void
AttacherFinit(int signum)
{
    exit(0);
}

static void
ReAttach(int signum)
{
    Attach(MSG_CONT);
}

static void pause(void)
{
    select(1, NULL, NULL, NULL, NULL);
}

static void
Attacher(void)
{
    signal(SIGHUP, AttacherFinit);
    signal(SIGCONT, ReAttach);
    while (1)
        pause();
}

static void
Detach(int suspend)
{
    struct win **pp;

    if (Detached)
        return;
    signal(SIGHUP, SIG_IGN);
    SetTTY(0, &OldMode);
    FinitTerm();
    disableRawMode(STDIN_FILENO);
    if (suspend) {
        Kill(AttacherPid, SIGTSTP);
    } else {
        for (pp = wtab; pp < wtab + MAXWIN; ++pp)
            if (*pp)
                RemoveUtmp((*pp)->slot);
        printf("\n[detached]\n");
        Kill(AttacherPid, SIGHUP);
        AttacherPid = 0;
    }
    close(0);
    close(1);
    close(2);
    //ioctl(DevTty, TIOCNOTTY, (char *)0);
    freetty();
    Detached = 1;
    do {
        ReceiveMsg(ServerSocket);
    } while (Detached);
    if (!suspend)
        for (pp = wtab; pp < wtab + MAXWIN; ++pp)
            if (*pp)
                (*pp)->slot = SetUtmp((*pp)->tty);
    signal(SIGHUP, SigHup);
}

static int
Kill(int pid, int sig)
{
    if (pid != 0)
        kill(pid, sig);
    return 0;
}

static int
GetSockName(void)
{
    int client;
    static char buf[2 * MAXSTR];

    if (!mflag && (SockName = getenv("STY")) != 0 && *SockName != '\0') {
        client = 1;
        setuid(getuid());
        setgid(getgid());
    } else {
        sprintf(buf, "%s.scr", Filename(GetTtyName()));
        SockName = buf;
        client = 0;
    }
    return client;
}

static int
MakeServerSocket(void)
{
    int s;
    struct sockaddr_un a;
    char *p;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, SockName);
    strcpy(a.sun_path, SockPath);
    if (connect(s, (struct sockaddr *)&a, strlen(SockPath) + 2) != -1) {
        p = Filename(SockPath);
        Msg(0, "You have already a screen running on %s.\n\
If it has been detached, try \"screen -r\".", p);
        /* NOTREACHED */
    }
    unlink(SockPath);
    if (bind(s, (struct sockaddr *)&a, strlen(SockPath) + 2) == -1)
        Msg(errno, "bind");
    chown(SockPath, getuid(), getgid());
    if (listen(s, 5) == -1)
        Msg(errno, "listen");
    return s;
}

static int
MakeClientSocket(int err)
{
    int s;
    struct sockaddr_un a;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, SockName);
    strcpy(a.sun_path, SockPath);
    if (connect(s, (struct sockaddr *)&a, strlen(SockPath) + 2) == -1) {
        if (err) {
            Msg(errno, "connect: %s", SockPath);
        } else {
            close(s);
            return -1;
        }
    }
    return s;
}

static int
SendCreateMsg(int s, int ac, char **av, int aflag)
{
    struct msg m;
    char *p;
    int len, n;

    m.type = MSG_CREATE;
    p = m.m.create.line;
    for (n = 0; ac > 0 && n < MAXARGS - 1; ++av, --ac, ++n) {
        len = strlen(*av) + 1;
        if (p + len >= m.m.create.line + MAXLINE)
            break;
        strcpy(p, *av);
        p += len;
    }
    m.m.create.nargs = n;
    m.m.create.aflag = aflag;
    if (getcwd(m.m.create.dir, sizeof(m.m.create.dir)) == 0)
        Msg(0, "%s", m.m.create.dir);
    if (write(s, (char *)&m, sizeof(m)) != sizeof(m))
        Msg(errno, "write");
    return 0;
}

static int
SendErrorMsg(char *fmt,...)
{
    int s;
    struct msg m;
    s = MakeClientSocket(1);
    m.type = MSG_ERROR;

    static va_list ap;
    va_start(ap, fmt);
    vsprintf(m.m.message, fmt, ap);
    va_end(ap);
    write(s, (char *)&m, sizeof(m));
    close(s);
    sleep(1);
    return 0;
}

static void
ReceiveMsg(int s)
{
    int ns;
    struct sockaddr_un a;
    int left, len = sizeof(a);
    struct msg m;
    char *p;

    if ((ns = accept(s, (struct sockaddr *)&a, (void *)&len)) == -1) {
        Msg(errno, "accept");
        return;
    }
    p = (char *)&m;
    left = sizeof(m);
    while (left > 0 && (len = read(ns, p, left)) > 0) {
        p += len;
        left -= len;
    }
    close(ns);
    if (len == -1)
        Msg(errno, "read");
    if (left > 0)
        return;
    switch (m.type) {
    case MSG_CREATE:
        if (!Detached)
            ExecCreate(&m);
        break;
    case MSG_CONT:
        if (m.m.attach.apid != AttacherPid || !Detached)
            break;              /* Intruder Alert */
        /* FALLTHROUGH */
    case MSG_ATTACH:
        if (Detached) {
            if (kill(m.m.attach.apid, 0) == 0 &&
                open(m.m.attach.tty, O_RDWR) == 0) {
                dup(0);
                dup(0);
                AttacherPid = m.m.attach.apid;
                Detached = 0;
                GetTTY(0, &OldMode);
                SetMode(&OldMode, &NewMode);
                SetTTY(0, &NewMode);
                Activate(wtab[CurrNum]);
            }
        } else {
            Kill(m.m.attach.apid, SIGHUP);
            Msg(0, "Not detached.");
        }
        break;
    case MSG_ERROR:
        Msg(0, "%s", m.m.message);
        break;
    default:
        Msg(0, "Invalid message (type %d).", m.type);
    }
}

static int
ExecCreate(struct msg *mp)
{
    char *args[MAXARGS];
    int n;
    char **pp = args, *p = mp->m.create.line;

    for (n = mp->m.create.nargs; n > 0; --n) {
        *pp++ = p;
        p += strlen(p) + 1;
    }
    *pp = 0;
    if ((n = MakeWindow(mp->m.create.line, args, mp->m.create.aflag, 0,
                        mp->m.create.dir)) != -1)
        SwitchWindow(n);
    return 0;
}

static void
ReadRc(char *fn)
{
    FILE *f;
    char *p, **pp, **ap;
    int argc, num, c;
    char buf[256];
    char *args[MAXARGS];
    int key;

    ap = args;
    if (access(fn, R_OK) == -1)
        return;
    if ((f = fopen(fn, "r")) == NULL)
        return;
    while (fgets(buf, 256, f) != NULL) {
        if ((p = strrchr(buf, '\n')))
            *p = '\0';
        if ((argc = Parse(fn, buf, ap)) == 0)
            continue;
        if (strcmp(ap[0], "escape") == 0) {
            p = ap[1];
            if (argc < 2 || strlen(p) != 2)
                Msg(0, "%s: two characters required after escape.", fn);
            Esc = *p++ - 0x40;  /* a control character is difficult to enter
                                 * with an editor */
            MetaEsc = *p;
        } else if (strcmp(ap[0], "chdir") == 0) {
            p = argc < 2 ? home : ap[1];
            if (chdir(p) == -1)
                Msg(errno, "%s", p);
        } else if (strcmp(ap[0], "mode") == 0) {
            if (argc != 2) {
                Msg(0, "%s: mode: one argument required.", fn);
            } else if (!IsNum(ap[1], 7)) {
                Msg(0, "%s: mode: octal number expected.", fn);
            } else
                sscanf(ap[1], "%o", &TtyMode);
        } else if (strcmp(ap[0], "bell") == 0) {
            if (argc != 2) {
                Msg(0, "%s: bell: one argument required.", fn);
            } else {
                if ((BellString = malloc(strlen(ap[1]) + 1)) == 0)
                    Msg(0, "Out of memory.");
                strcpy(BellString, ap[1]);
            }
        } else if (strcmp(ap[0], "screen") == 0) {
            num = 0;
            if (argc > 1 && IsNum(ap[1], 10)) {
                num = atoi(ap[1]);
                if (num < 0 || num > MAXWIN - 1)
                    Msg(0, "%s: illegal screen number %d.", fn, num);
                --argc;
                ++ap;
            }
            if (argc < 2) {
                ap[1] = ShellProg;
                argc = 2;
            }
            ap[argc] = 0;
            MakeWindow(ap[1], ap + 1, 0, num, (char *)0);
        } else if (strcmp(ap[0], "bind") == 0) {
            p = ap[1];
            if (argc < 2 || *p == '\0')
                Msg(0, "%s: key expected after bind.", fn);
            if (p[1] == '\0') {
                key = *p;
            } else if (p[0] == '^' && p[1] != '\0' && p[2] == '\0') {
                c = p[1];
                if (isupper(c))
                    p[1] = tolower(c);
                key = Ctrl(c);
            } else if (IsNum(p, 7)) {
                sscanf(p, "%o", &key);
            } else {
                Msg(0,
                  "%s: bind: character, ^x, or octal number expected.", fn);
            }
            if (argc < 3) {
                ktab[key].type = 0;
            } else {
                for (pp = KeyNames; *pp; ++pp)
                    if (strcmp(ap[2], *pp) == 0)
                        break;
                if (*pp) {
                    ktab[key].type = pp - KeyNames + 1;
                } else {
                    ktab[key].type = KEY_CREATE;
                    ktab[key].args = SaveArgs(argc - 2, ap + 2);
                }
            }
        } else
            Msg(0, "%s: unknown keyword \"%s\".", fn, ap[0]);
    }
    fclose(f);
}

static int
Parse(char *fn, char *buf, char **args)
{
    char *p = buf, **ap = args;
    int delim, argc = 0;

    argc = 0;
    for (;;) {
        while (*p && (*p == ' ' || *p == '\t'))
            ++p;
        if (*p == '\0' || *p == '#')
            return argc;
        if (argc > MAXARGS - 1)
            Msg(0, "%s: too many tokens.", fn);
        delim = 0;
        if (*p == '"' || *p == '\'') {
            delim = *p;
            *p = '\0';
            ++p;
        }
        ++argc;
        *ap = p;
        ++ap;
        while (*p && !(delim ? *p == delim : (*p == ' ' || *p == '\t')))
            ++p;
        if (*p == '\0') {
            if (delim)
                Msg(0, "%s: Missing quote.", fn);
            else
                return argc;
        }
        *p++ = '\0';
    }
}

static char **
SaveArgs(int argc, char **argv)
{
    char **ap, **pp;

    if ((pp = ap = (char **)malloc((argc + 1) * sizeof(char **))) == 0)
        Msg(0, "Out of memory.");
    while (argc--) {
        if ((*pp = malloc(strlen(*argv) + 1)) == 0)
            Msg(0, "Out of memory.");
        strcpy(*pp, *argv);
        ++pp;
        ++argv;
    }
    *pp = 0;
    return ap;
}

static int
MakeNewEnv(void)
{
    char **op, **np = NewEnv;
    static char buf[MAXSTR];

    if (strlen(SockName) > MAXSTR - 5)
        SockName = "?";
    sprintf(buf, "STY=%s", SockName);
    *np++ = buf;
    *np++ = Term;
    np += 2;
    for (op = environ; *op; ++op) {
        if (np == NewEnv + MAXARGS - 1)
            break;
        if (!IsSymbol(*op, "TERM") && !IsSymbol(*op, "TERMCAP")
            && !IsSymbol(*op, "STY"))
            *np++ = *op;
    }
    *np = 0;
    return 0;
}

static int
IsSymbol(char *e, char *s)
{
    char *p;
    int n;

    for (p = e; *p && *p != '='; ++p);
    if (*p) {
        *p = '\0';
        n = strcmp(e, s);
        *p = '=';
        return n == 0;
    }
    return 0;
}

void
Msg(int err, char *fmt,...)
{
    static va_list ap;
    char buf[1024];
    char *p = buf;
    va_start(ap, fmt);
    vsprintf(p, fmt, ap);
    va_end(ap);
    if (err) {
        p += strlen(p);
        sprintf(p, ": Error %d, %s", err, strerror(err));
    }
    if (HasWindow) {
        MakeStatus(buf, curr);
    } else {
        printf("%s\r\n", buf);
    }
}

int
bclear(char *p, int n)
{
    bcopy(blank, p, n);
    return 0;
}

static char *
Filename(char *s)
{
    char *p;

    p = s + strlen(s) - 1;
    while (p >= s && *p != '/')
        --p;
    return ++p;
}

static int
IsNum(char *s, int base)
{
    for (base += '0'; *s; ++s)
        if (*s < '0' || *s > base)
            return 0;
    return 1;
}

static char *
MakeBellMsg(int n)
{
    static char buf[MAXSTR];
    char *p = buf, *s;

    for (s = BellString; *s && p < buf + MAXSTR - 1; s++)
        *p++ = (*s == '%') ? n + '0' : *s;
    *p = '\0';
    return buf;
}

#ifdef UTMP
static int
InitUtmp(void)
{
    struct passwd *p;

    if ((utmpf = open(UtmpName, O_WRONLY)) == -1) {
        if (errno != EACCES)
            Msg(errno, UtmpName);
        return;
    }
    if ((LoginName = getlogin()) == 0 || LoginName[0] == '\0') {
        if ((p = getpwuid(getuid())) == 0)
            return;
        LoginName = p->pw_name;
    }
    utmp = 1;
}

static int
SetUtmp(char *name)
{
    char *p;
    struct ttyent *tp;
    int slot = 1;
    struct utmp u;

    if (!utmp)
        return 0;
    if (p = strrchr(name, '/'))
        ++p;
    else
        p = name;
    setttyent();
    while ((tp = getttyent()) != NULL && strcmp(p, tp->ty_name) != 0)
        ++slot;
    if (tp == NULL)
        return 0;
    strncpy(u.ut_line, p, 8);
    strncpy(u.ut_user, LoginName, 8);
    u.ut_host[0] = '\0';
    time(&u.ut_time);
    lseek(utmpf, (long)(slot * sizeof(u)), 0);
    write(utmpf, (char *)&u, sizeof(u));
    return slot;
}

static int
RemoveUtmp(int slot)
{
    struct utmp u;

    if (slot) {
        bzero((char *)&u, sizeof(u));
        lseek(utmpf, (long)(slot * sizeof(u)), 0);
        write(utmpf, (char *)&u, sizeof(u));
    }
}

#else
static int
InitUtmp(void)
{
    return 0;
}

static int
SetUtmp(char *name)
{
    return 0;
}

static int
RemoveUtmp(int slot)
{
    return 0;
}

#endif

#ifndef GETTTYENT

static void
setttyent(void)
{
    struct stat s;
    int f;
    char *p, *ep;

    if (ttnext) {
        ttnext = tt;
        return;
    }
    if ((f = open(ttys, O_RDONLY)) == -1 || fstat(f, &s) == -1)
        Msg(errno, ttys);
    if ((tt = malloc(s.st_size + 1)) == 0)
        Msg(0, "Out of memory.");
    if (read(f, tt, s.st_size) != s.st_size)
        Msg(errno, ttys);
    close(f);
    for (p = tt, ep = p + s.st_size; p < ep; ++p)
        if (*p == '\n')
            *p = '\0';
    *p = '\0';
    ttnext = tt;
}

static struct ttyent *
getttyent(void)
{
    static struct ttyent t;

    if (*ttnext == '\0')
        return NULL;
    t.ty_name = ttnext + 2;
    ttnext += strlen(ttnext) + 1;
    return &t;
}

#endif

#ifdef LOADAV

static int
InitKmem(void)
{
    if ((kmemf = open(KmemName, O_RDONLY)) == -1)
        return;
    nl[0].n_name = AvenrunSym;
    nlist(UnixName, nl);
    if (nl[0].n_type == 0 || nl[0].n_value == 0)
        return;
    avenrun = 1;
}

static int
GetAvenrun(void)
{
    if (lseek(kmemf, nl[0].n_value, 0) == -1)
        return 0;
    if (read(kmemf, loadav, sizeof(loadav)) != sizeof(loadav))
        return 0;
    return 1;
}

#endif

#ifndef USEBCOPY
#if 0
bcopy(s1, s2, len) register char *s1, *s2;
register len; {
    if (s1 < s2 &&
        s2 < s1 + len) {
        s1 += len;
        s2 += len;
        while (len-- > 0) {
            --s2 = *--s1;
        }
    } else {
        while (len-- > 0) {
            s2++ = *s1++;
        }
    }
}
#endif
#endif

int
gethostname(char *host, size_t size)
{
    strncpy(host, "elks", size);
    host[size - 1] = '\0';
    return 0;
}

static int
enableRawMode(int fd)
{
    struct termios raw;

    if (!isatty(STDIN_FILENO))
        goto fatal;
#if 0
    if (!atexit_registered) {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
#endif
    if (tcgetattr(fd, &orig_termios) == -1)
        goto fatal;

    raw = orig_termios;         /* modify the original mode */
    /*
     * input modes: no break, no CR to NL, no parity check, no strip char, no
     * start/stop output control.
     */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /*
     * local modes - echo nothing, canonical off, no extended functions, no
     * signal chars (^Z,^C)
     */
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    /*
     * control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout.
     */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;        /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
        goto fatal;
    rawmode = 1;
    return 0;

fatal:
    //errno = ENOTTY;
    return -1;
}

static void
disableRawMode(int fd)
{
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd, TCSAFLUSH, &orig_termios) != -1)
        rawmode = 0;
}

static void
brktty(void)
{
    setsid();                   /* will break terminal affiliation */
}

static void
freetty(void)
{
    brktty();
    close(0);
    close(1);
    close(2);
}

void
exit_with_usage(char *myname)
{
    printf("Use: %s [-opts] [cmd [args]]\n", myname);
    printf(" or: %s -r [ttyX.scr]\n\nOptions:\n", myname);
    printf("-a           Force all capabilities into each window's termcap\n");
    printf("-e xy        Change command characters\n");
    printf("-f           Flow control on\n");
    printf("-n           Flow control off\n");
    printf("-l           Do nothing, just list the SockDir\n");
    /* printf("-r           Reattach to a detached screen process\n"); */
    exit(1);
}

int
display_help(void)
{
    /*
     * Ctrl+a d   Detach from the current screen session, and leave it
     * running.\n\r\ Use screen -r to resume\n\r\
     */
    printf("\n\r\
    Ctrl+a ?   Display available commands\n\r\
    Ctrl+a v   Show the version\n\r\
    Ctrl+a w   Window list\n\r\
    Ctrl+a 0-9 Open window 0-9\n\r\
    Ctrl+a c   Create a new window (with shell)\n\r\
    Ctrl-a n   Switch to the Next window\n\r\
    Ctrl-a p   Switch to the Previous window\n\r\
    Ctrl+a l   Fully refresh current window\n\r\
    Ctrl+a k   Kill (Destroy) the current window.\n\r\
    Ctrl-a K   Kill all windows and terminate screen(quit)\n\r\
    Ctrl-a t   Show the load average\n\r\
    Ctrl-a .   Write out a .termcap file\n\r");
    return 0;
}

void
dbgmsg(int n)
{
    printf("dbg:%d\n", n);
    fflush(stdout);
    sleep(1);
}
