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

static int SigHandler();
static int SigChld();
static int DoWait();
static void CheckWindows();
static int ProcessInput(char *buf, int len);
static void SwitchWindow(int n);
static int SetCurrWindow(int n);
int NextWindow();
static int PreviousWindow();
static int FreeWindow(struct win *wp);
static int ShowWindows();
static void DisplayLine(char *, char *, char *, char *, char *, char *, int, int, int);
static void RedisplayLine(char *, char *, char *, int, int, int);

static int CheckSockName(int client);
static int MakeServerSocket();
static int MakeClientSocket(int err);
static int SendCreateMsg(int s, int ac, char **av, int aflag);
static int SendErrorMsg(char *fmt,...);
static void ReceiveMsg(int s);
static int ExecCreate(struct msg *mp);
static void ReadRc(char *fn);
static int Parse(char *fn, char *buf, char **args);
static char **SaveArgs(int argc, char **argv);
static int MakeNewEnv();
static int IsSymbol(char *e, char *s);
void Msg(int err, char *fmt,...);
int bclear(char *p, int n);
static char *Filename(char *s);
static int IsNum(char *s, int base);

static int RemoveUtmp(int slot);
static int SetUtmp(char *name);
static int InitUtmp();
static int MoreWindows();
int MakeWindow(char *prog, char **args, int aflag, int StartAt, char *dir);
static int GetSockName();
static int Kill(int pid, int sig);
static void Attacher();
static int Attach(int how);
static void Detach(int suspend);
static int SetTTY(int fd, struct mode *mp);
static int GetTTY(int fd, struct mode *mp);
static int ShowInfo();
static void screen_execvpe(char *prog, char **args, char **env);
static void WriteFile(int dump);
static void KillWindow(struct win **pp);
static int Finit();
static int InitKeytab();
int InitTerm();
int FinitTerm();
void WriteString(struct win *wp, char *buf, int len);
int Activate(struct win *wp);
void DoESC(int c, int intermediate);
int ResetScreen(struct win *p);
void RemoveStatus(struct win *p);
int MakeStatus(char *msg, struct win *wp);
int gethostname(char *host, int size);

static int enableRawMode(int fd);
static void disableRawMode(int fd);

static void brktty();
static void freetty();

void exit_with_usage(char *myname);
int display_help();

void dbgmsg(int n);
