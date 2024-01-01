/*
 * Copyright (c) 1987,1988 Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute this
 * software, provided that no attempt is made to gain profit from it, the
 * author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is not
 * modified.
 */

enum state_t {
    LIT,                        /* Literal input */
    ESC,                        /* Start of escape sequence */
    STR,                        /* Start of control string */
    TERM,                       /* ESC seen in control string */
    CSI,                        /* Reading arguments in "CSI Pn ; Pn ; ... ;
                                 * XXX" */
    PRIN,                       /* Printer mode */
    PRINESC,                    /* ESC seen in printer mode */
    PRINCSI,                    /* CSI seen in printer mode */
    PRIN4                       /* CSI 4 seen in printer mode */
};

enum string_t {
    NONE,
    DCS,                        /* Device control string */
    OSC,                        /* Operating system command */
    APC,                        /* Application program command */
    PM,                         /* Privacy message */
};

#define MAXSTR       128
#define	MAXARGS      64
#define IOSIZE       80

struct win {
    int wpid;
    int ptyfd;
    int aflag;
    char outbuf[IOSIZE];
    int outlen;
    char cmd[MAXSTR];
    char tty[MAXSTR];
    int args[MAXARGS];
    char GotArg[MAXARGS];
    int NumArgs;
    int slot;
    char **image;
    char **attr;
    char **font;
    int LocalCharset;
    int charsets[4];
    int ss;
    int active;
    int x, y;
    char LocalAttr;
    int saved;
    int Saved_x, Saved_y;
    char SavedLocalAttr;
    int SavedLocalCharset;
    int SavedCharsets[4];
    int top, bot;
    int wrap;
    int origin;
    int insert;
    int keypad;
    enum state_t state;
    enum string_t StringType;
    char string[MAXSTR];
    char *stringp;
    char *tabs;
    int vbwait;
    int bell;
};

#define MAXLINE 1024

#define MSG_CREATE    0
#define MSG_ERROR     1
#define MSG_ATTACH    2
#define MSG_CONT      3

struct msg {
    int type;
    union {
        struct {
            int aflag;
            int nargs;
            char line[MAXLINE];
            char dir[1024];
        } create;
        struct {
            int apid;
            char tty[1024];
        } attach;
        char message[MAXLINE];
    } m;
};

struct mode {
#ifdef BSD_TTY
    struct sgttyb m_ttyb;
    struct tchars m_tchars;
    struct ltchars m_ltchars;
    int m_ldisc;
    int m_lmode;
#else
    int dummy;
#endif
};

static int SigHandler(void);
static int SigChld(void);
static int DoWait(void);
static void CheckWindows(void);
static int ProcessInput(char *buf, int len);
static void SwitchWindow(int n);
static int SetCurrWindow(int n);
int NextWindow(void);
static int PreviousWindow(void);
static int FreeWindow(struct win *wp);
static int ShowWindows(void);
static void DisplayLine(char *, char *, char *, char *, char *, char *, int, int, int);
static void RedisplayLine(char *, char *, char *, int, int, int);

static int CheckSockName(int client);
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
void Msg(int err, char *fmt,...);
int bclear(char *p, int n);
static char *Filename(char *s);
static int IsNum(char *s, int base);

static int RemoveUtmp(int slot);
static int SetUtmp(char *name);
static int InitUtmp(void);
static int MoreWindows(void);
int MakeWindow(char *prog, char **args, int aflag, int StartAt, char *dir);
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
static int Finit(void);
static int InitKeytab(void);
int InitTerm(void);
int FinitTerm(void);
void WriteString(struct win *wp, char *buf, int len);
int Activate(struct win *wp);
void DoESC(int c, int intermediate);
int ResetScreen(struct win *p);
void RemoveStatus(struct win *p);
int MakeStatus(char *msg, struct win *wp);
int gethostname(char *host, int size);

static int enableRawMode(int fd);
static void disableRawMode(int fd);

static void brktty(void);
static void freetty(void);

void exit_with_usage(char *myname);
int display_help(void);

void dbgmsg(int n);
