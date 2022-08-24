#ifndef __UNIKEY__H
#define __UNIKEY__H

/*
 * ANSI to Unicode keyboard mapping
 */

enum ttyflags {
    MouseTracking = 1,              /* mouse button tracking */
    FullMouseTracking = 2,          /* track mouse position events */
    CatchISig = 4,                  /* catch SIGINT and exit by default */
    Utf8 = 8,                       /* set termios IUTF8 */
    ExitLastLine = 16,              /* on exit, cursor position to bottom */
    FullBuffer                      /* fully buffered output */
};

/* tty.c - tty termios and mouse management */
int tty_init(enum ttyflags flags);
void tty_enable_unikey(void);
void tty_restore(void);
void tty_fullbuffer(void);
void tty_linebuffer(void);
int tty_getsize(int *cols, int *rows);
extern int iselksconsole;

/* tty-cp437.c - display cp437 characters */
char *tty_allocate_screen(int cols, int lines);
void tty_output_screen(int flush);
void tty_setfgpalette(const int *pal16, const int *pal256);

/* unikey.c - recognize ANSI input */

/* check and convert from ANSI keyboard sequence to unicode key value */
int ansi_to_unikey(char *buf, int n);

/* check and convert from ANSI mouse sequence to unicode mouse value */
int ansi_to_unimouse(char *buf, int n, int *x, int *y, int *modkeys, int *status);
const char *describemouseevent(int e);
extern int kDoubleClickTime;

/* Check and respond to ANSI DSR (device status report). */
int ansi_dsr(char *buf, int n, int *cols, int *rows);

/* return keyname */
char *unikeyname(int k);

/* state machine conversion of UTF-8 byte sequence to Rune */
int stream_to_rune(unsigned int ch);

/* Reads single keystroke or control sequence from character device */
int readansi(int fd, char *p, int n);

#define bsr(x)  ((x)? (__builtin_clz(x) ^ ((sizeof(int) * 8) -1)): 0)
#if 0
int bsr(int n)
{
    if (n == 0) return 0;   /* avoid incorrect result of 31 returned! */
    return (__builtin_clz(n) ^ ((sizeof(int) * 8) - 1));
}
#endif

/* defines from Cosmopolitan - see copyright in unikey.c */
#define ThomPikeCont(x)     (0200 == (0300 & (x)))
#define ThomPikeByte(x)     ((x) & (((1 << ThomPikeMsb(x)) - 1) | 3))
#define ThomPikeLen(x)      (7 - ThomPikeMsb(x))
#define ThomPikeMsb(x)      ((255 & (x)) < 252 ? bsr(255 & ~(x)) : 1)
#define ThomPikeMerge(x, y) ((x) << 6 | (077 & (y)))

/* when adding key values, add to unikeyname[] in unikey.c also */
typedef enum {
    /* ASCII range */
    kBackSpace = 8,
    kTab = 9,
    kEscKey = 27,
    kDel = 127,

    /* Unicode private use range 0xF000 - 0xF1FF */
    /* max key group size is 32 (0x20) */
#define kKeyFirst   ((unsigned)kUpArrow)
    kUpArrow = 0xF000,
    kDownArrow,
    kLeftArrow,
    kRightArrow,
    kInsert,
    kDelete,
    kHome,
    kEnd,
    kPageUp,
    kPageDown,
    /* leave out keypad keys for now */

    kMouseLeftDown,
    kMouseMiddleDown,
    kMouseRightDown,
    kMouseLeftUp,
    kMouseMiddleUp,
    kMouseRightUp,
    kMouseLeftDrag,
    kMouseMiddleDrag,
    kMouseRightDrag,
    kMouseWheelUp,
    kMouseWheelDown,
    kMouseMotion,
    kMouseLeftDoubleClick,

    kF1     = 0xF020,
    kF2,
    kF3,
    kF4,
    kF5,
    kF6,
    kF7,
    kF8,
    kF9,
    kF10,
    kF11,
    kF12,

    kCtrl   = 0x0080,
    kCtrlF1 = kF1 + kCtrl,      /* = 0xF0A0 */
    kCtrlF2,
    kCtrlF3,
    kCtrlF4,
    kCtrlF5,
    kCtrlF6,
    kCtrlF7,
    kCtrlF8,
    kCtrlF9,
    kCtrlF10,
    kCtrlF11,
    kCtrlF12,

    /* TODO: entries for Alt-kUpArrow */

    kAlt   = 0x0100,
    kAltF1 = kF1 + kAlt,        /* = 0xF120 */
    kAltF2,
    kAltF3,
    kAltF4,
    kAltF5,
    kAltF6,
    kAltF7,
    kAltF8,
    kAltF9,
    kAltF10,
    kAltF11,
    kAltF12,

    kAltA = kF1 + kAlt + 0x20,  /* = 0xF140 */
    kAltB,
    kAltC,
    kAltD,
    kAltE,
    kAltF,
    kAltG,
    kAltH,
    kAltI,
    kAltJ,
    kAltK,
    kAltL,
    kAltM,
    kAltN,
    kAltO,
    kAltP,
    kAltQ,
    kAltR,
    kAltS,
    kAltT,
    kAltU,
    kAltV,
    kAltW,
    kAltX,
    kAltY,
    kAltZ,
    kAlt0,
    kAlt1,
    kAlt2,
    kAlt3,
    kAlt4,
    kAlt5,
    kAlt6,
    kAlt7,
    kAlt8,
    kAlt9,

    kShift = 0x0200         /* bit value only, not ORed into private use range */
} unikey;

struct unikeyname {
    unikey  key;
    char *  name;
};

extern struct unikeyname unikeynames[];

#endif
