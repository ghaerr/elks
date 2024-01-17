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

void Msg(int err, char *fmt,...);
int bclear(char *p, int n);

int InitTerm(void);
int FinitTerm(void);
void WriteString(struct win *wp, char *buf, int len);
int Activate(struct win *wp);
void DoESC(int c, int intermediate);
int ResetScreen(struct win *p);
void RemoveStatus(struct win *p);
int MakeStatus(char *msg, struct win *wp);
int gethostname(char *host, size_t size);

void exit_with_usage(char *myname);
int display_help(void);

void dbgmsg(int n);
