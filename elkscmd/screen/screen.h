/* Copyright (c) 1987,1988 Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

enum state_t {
    LIT,         /* Literal input */
    ESC,         /* Start of escape sequence */
    STR,         /* Start of control string */
    TERM,        /* ESC seen in control string */
    CSI,         /* Reading arguments in "CSI Pn ; Pn ; ... ; XXX" */
    PRIN,        /* Printer mode */
    PRINESC,     /* ESC seen in printer mode */
    PRINCSI,     /* CSI seen in printer mode */
    PRIN4        /* CSI 4 seen in printer mode */
};

enum string_t {
    NONE,
    DCS,         /* Device control string */
    OSC,         /* Operating system command */
    APC,         /* Application program command */
    PM,          /* Privacy message */
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

static SigHandler(void);
static SigChld(void);
static DoWait(void);
static void CheckWindows(void);
static ProcessInput(char *buf, int len);
static void SwitchWindow(int n);
static SetCurrWindow(int n);
int NextWindow(void);
static PreviousWindow(void);
static FreeWindow(struct win *wp);
static ShowWindows(void);
/* int OpenPTY(int n); */

static void DisplayLine __P((char*, char*, char*, char*, char*, char*, int, int, int));
static void RedisplayLine __P((char*, char*, char*, int, int, int));

static CheckSockName(int client);
static MakeServerSocket(void);
static MakeClientSocket(int err);
static SendCreateMsg(int s, int ac, char **av, int aflag);
static SendErrorMsg(char *fmt, ...);
static void ReceiveMsg(int s);
static ExecCreate(struct msg *mp);
static void ReadRc(char *fn);
static Parse(char *fn, char *buf, char **args);
static char **SaveArgs(register argc, register char **argv);
static MakeNewEnv(void);
static IsSymbol(register char *e, register char *s);
void   Msg(int err, char *fmt, ...);
int    bclear(char *p, int n);
static char *Filename(char *s);
static IsNum(register char *s, register base);

static RemoveUtmp (int slot);
static SetUtmp (char *name);
static InitUtmp ();
static MoreWindows(void);
int MakeWindow(char *prog, char **args, int aflag, int StartAt, char *dir);
static int GetSockName(void);
static Kill(int pid, int sig);
static Attacher(void);
static Attach(int how);
static void Detach(int suspend);
static SetTTY(int fd, struct mode *mp);
static GetTTY(int fd, struct mode *mp);
static ShowInfo(void);
static void execvpe(char *prog, char **args, char **env);
static void WriteFile(int dump);
static KillWindow(struct win **pp);
static Finit(void);
static InitKeytab(void);
int InitTerm(void);
int FinitTerm(void);
void WriteString(struct win *wp, register char *buf, int len);
int Activate(struct win *wp);
void DoESC(int c, int intermediate);
int ResetScreen(register struct win *p);
void RemoveStatus(struct win *p);
int MakeStatus(char *msg, struct win *wp);
int gethostname (char *host, int size);

static int enableRawMode(int fd);
static void disableRawMode(int fd);

static void brktty(void);
static void freetty(void);
void exit_with_usage( char* myname);
int display_help(void);

void dbgmsg(int n);
