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

char AnsiVersion[] = "ansi 2.0a (ELKS) 25-Apr-2020";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <termcap.h>
#include "screen.h"
#include "ansi.h"

#define A_SO     0x1            /* Standout mode */
#define A_US     0x2            /* Underscore mode */
#define A_BL     0x4            /* Blinking */
#define A_BD     0x8            /* Bold mode */
#define A_DI    0x10            /* Dim mode */
#define A_RV    0x20            /* Reverse mode */
#define A_MAX   0x20

/* Types of movement used by Goto() */
enum move_t {
    M_NONE,
    M_UP,
    M_DO,
    M_LE,
    M_RI,
    M_RW,
    M_CR,
};

#define EXPENSIVE    1000

#define G0           0
#define G1           1
#define G2           2
#define G3           3

#define ASCII        0

extern char *getenv(), *tgetstr(), *tgoto();

static void RedisplayLine(char *os, char *oa, char *of, int y, int from, int to);
static void DisplayLine(char *os, char *oa, char *of, char *s, char *as, char *fs, int y, int from, int to);

int rows, cols;
int status;
int flowctl;
char Term[] = "TERM=screen";
char Termcap[1024];
char *blank;
char PC;
int ISO2022;
time_t TimeDisplayed;

static char tbuf[1024], tentry[1024];
static char *tp = tentry;
char *TI, *TE, *BL, *VB, *BC, *CR, *NL, *CL, *IS, *CM;
static char *US, *UE, *SO, *SE, *CE, *CD, *DO, *SR, *SF, *AL;
char *CS, *DL, *DC, *IC, *IM, *EI, *UP, *ND, *KS, *KE;
static char *MB, *MD, *MH, *MR, *ME, *PO, *PF;
static char *CDC, *CDL, *CAL;
static int AM;
static char GlobalAttr, TmpAttr, GlobalCharset, TmpCharset;
static char *OldImage, *OldAttr, *OldFont;
static int last_x, last_y;
static struct win *curr;
static int display = 1;
static int StrCost;
static int UPcost, DOcost, LEcost, NDcost, CRcost, IMcost, EIcost;
static int tcLineLen = 100;
static char *null;
static int StatLen;
static int insert;
static int keypad;

static char *KeyCaps[] = {
    "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8", "k9",
    "kb", "kd", "kh", "kl", "ko", "kr", "ku",
    0
};

static char TermcapConst[] = "TERMCAP=\
SC|screen|VT 100/ANSI X3.64 virtual terminal|\\\n\
\t:DO=\\E[%dB:LE=\\E[%dD:RI=\\E[%dC:UP=\\E[%dA:bs:bt=\\E[Z:\\\n\
\t:cd=\\E[J:ce=\\E[K:cl=\\E[2J\\E[H:cm=\\E[%i%d;%dH:ct=\\E[3g:\\\n\
\t:do=\\E[B:nd=\\E[C:pt:rc=\\E8:rs=\\Ec:sc=\\E7:st=\\EH:up=\\E[A:le=^H:";


int
InitTerm(void)
{
    char *s;

    if ((s = getenv("TERM")) == 0)
        Msg(0, "No TERM in environment.");
    if (tgetent(tbuf, s) != 1)
        Msg(0, "Cannot find termcap entry for %s.", s);
    cols = tgetnum("co");
    rows = tgetnum("li");
    if (cols <= 0)
        cols = 80;
    if (rows <= 0)
        rows = 24;
    if (tgetflag("hc"))
        Msg(0, "You can't run screen on a hardcopy terminal.");
    if (tgetflag("os"))
        Msg(0, "You can't run screen on a terminal that overstrikes.");
    if (tgetflag("ns"))
        Msg(0, "Terminal must support scrolling.");
    if (!(CL = tgetstr("cl", &tp)))
        Msg(0, "Clear screen capability required.");
    if (!(CM = tgetstr("cm", &tp)))
        Msg(0, "Addressable cursor capability required.");
    if ((s = tgetstr("ps", &tp)))
        PC = s[0];
    flowctl = !tgetflag("NF");
    AM = tgetflag("am");
    if (tgetflag("LP"))
        AM = 0;
    TI = tgetstr("ti", &tp);
    TE = tgetstr("te", &tp);
    if (!(BL = tgetstr("bl", &tp)))
        BL = "\007";
    VB = tgetstr("vb", &tp);
    if (!(BC = tgetstr("bc", &tp))) {
        if (tgetflag("bs"))
            BC = "\b";
        else
            BC = tgetstr("le", &tp);
    }
    if (!(CR = tgetstr("cr", &tp)))
        CR = "\r";
    if (!(NL = tgetstr("nl", &tp)))
        NL = "\r\n";            /* add CR for ELKS */
    IS = tgetstr("is", &tp);
    if (tgetnum("sg") <= 0) {
        US = tgetstr("us", &tp);
        UE = tgetstr("ue", &tp);
        SO = tgetstr("so", &tp);
        SE = tgetstr("se", &tp);
        MB = tgetstr("mb", &tp);
        MD = tgetstr("md", &tp);
        MH = tgetstr("mh", &tp);
        MR = tgetstr("mr", &tp);
        ME = tgetstr("me", &tp);
        /*
         * Does ME also reverse the effect of SO and/or US?  This is not
         * clearly specified by the termcap manual. Anyway, we should at
         * least look whether ME and SE/UE are equal:
         */
        if (SE && UE && ME && (strcmp(SE, UE) == 0 || strcmp(ME, UE) == 0))
            UE = 0;
        if (SE && ME && strcmp(SE, ME) == 0)
            SE = 0;
    }
    CE = tgetstr("ce", &tp);
    CD = tgetstr("cd", &tp);
    if (!(DO = tgetstr("do", &tp)))
        DO = NL;
    UP = tgetstr("up", &tp);
    ND = tgetstr("nd", &tp);
    SR = tgetstr("sr", &tp);
    if (!(SF = tgetstr("sf", &tp)))
        SF = NL;
    AL = tgetstr("al", &tp);
    DL = tgetstr("dl", &tp);
    CS = tgetstr("cs", &tp);
    DC = tgetstr("dc", &tp);
    IC = tgetstr("ic", &tp);
    CDC = tgetstr("DC", &tp);
    CDL = tgetstr("DL", &tp);
    CAL = tgetstr("AL", &tp);
    IM = tgetstr("im", &tp);
    EI = tgetstr("ei", &tp);
    if (tgetflag("in"))
        IC = IM = 0;
    if (IC && IC[0] == '\0')
        IC = 0;
    if (IM && IM[0] == '\0')
        IM = 0;
    if (EI && EI[0] == '\0')
        EI = 0;
    KS = tgetstr("ks", &tp);
    KE = tgetstr("ke", &tp);
    ISO2022 = tgetflag("G0");
    PO = tgetstr("po", &tp);
    if (!(PF = tgetstr("pf", &tp)))
        PO = 0;
    blank = malloc(cols);
    null = malloc(cols);
    OldImage = malloc(cols);
    OldAttr = malloc(cols);
    OldFont = malloc(cols);
    if (!(blank && null && OldImage && OldAttr && OldFont))
        Msg(0, "Out of memory.");
    MakeBlankLine(blank, cols);
    bzero(null, cols);
    UPcost = CalcCost(UP);
    DOcost = CalcCost(DO);
    LEcost = CalcCost(BC);
    NDcost = CalcCost(ND);
    CRcost = CalcCost(CR);
    IMcost = CalcCost(IM);
    EIcost = CalcCost(EI);
    PutStr(IS);
    PutStr(TI);
    PutStr(CL);
    return 0;
}

int
FinitTerm(void)
{
    PutStr(TE);
    PutStr(IS);
    return 0;
}

static int
AddCap(char *s)
{
    int n;

    if (tcLineLen + (n = strlen(s)) > 55) {
        strcat(Termcap, "\\\n\t:");
        tcLineLen = 0;
    }
    strcat(Termcap, s);
    tcLineLen += n;
    return 0;
}

char *
MakeTermcap(int aflag)
{
    char buf[1024];
    char **pp, *p;

    strcpy(Termcap, TermcapConst);
    sprintf(buf, "li#%d:co#%d:", rows, cols);
    AddCap(buf);
    if (VB)
        AddCap("vb=\\E[?5h\\E[?5l:");
    if (US) {
        AddCap("us=\\E[4m:");
        AddCap("ue=\\E[24m:");
    }
    if (SO) {
        AddCap("so=\\E[3m:");
        AddCap("se=\\E[23m:");
    }
    if (MB)
        AddCap("mb=\\E[5m:");
    if (MD)
        AddCap("md=\\E[1m:");
    if (MH)
        AddCap("mh=\\E[2m:");
    if (MR)
        AddCap("mr=\\E[7m:");
    if (MB || MD || MH || MR)
        AddCap("me=\\E[0m:");
    if ((CS && SR) || AL || CAL || aflag) {
        AddCap("sr=\\EM:");
        AddCap("al=\\E[L:");
        AddCap("AL=\\E[%dL:");
    }
    if (CS || DL || CDL || aflag) {
        AddCap("dl=\\E[M:");
        AddCap("DL=\\E[%dM:");
    }
    if (CS)
        AddCap("cs=\\E[%i%d;%dr:");
    if (DC || CDC || aflag) {
        AddCap("dc=\\E[P:");
        AddCap("DC=\\E[%dP:");
    }
    if (IC || IM || aflag) {
        AddCap("im=\\E[4h:");
        AddCap("ei=\\E[4l:");
        AddCap("ic=:");
        AddCap("IC=\\E[%d@:");
    }
    if (KS)
        AddCap("ks=\\E=:");
    if (KE)
        AddCap("ke=\\E>:");
    if (ISO2022)
        AddCap("G0:");
    if (PO) {
        AddCap("po=\\E[5i:");
        AddCap("pf=\\E[4i:");
    }
    for (pp = KeyCaps; *pp; ++pp)
        if ((p = tgetstr(*pp, &tp))) {
            MakeString(*pp, buf, p);
            AddCap(buf);
        }
    return Termcap;
}

static int
MakeString(char *cap, char *buf, char *s)
{
    char *p = buf;
    unsigned c;

    *p++ = *cap++;
    *p++ = *cap;
    *p++ = '=';
    while ((c = *s++)) {
        switch (c) {
        case '\033':
            *p++ = '\\';
            *p++ = 'E';
            break;
        case ':':
            sprintf(p, "\\072");
            p += 4;
            break;
        case '^':
        case '\\':
            *p++ = '\\';
            *p++ = c;
            break;
        default:
            if (c >= 200) {
                sprintf(p, "\\%03o", c & 0377);
                p += 4;
            } else if (c < ' ') {
                *p++ = '^';
                *p++ = c + '@';
            } else
                *p++ = c;
        }
    }
    *p++ = ':';
    *p = '\0';
    return 0;
}

int
Activate(struct win *wp)
{
    RemoveStatus(wp);
    curr = wp;
    display = 1;
    NewRendition(GlobalAttr, curr->LocalAttr);
    GlobalAttr = curr->LocalAttr;
    NewCharset(GlobalCharset, curr->charsets[curr->LocalCharset]);
    GlobalCharset = curr->charsets[curr->LocalCharset];
    if (CS)
        PutStr(tgoto(CS, curr->bot, curr->top));
    Redisplay();
    KeypadMode(curr->keypad);
    return 0;
}

int
ResetScreen(struct win *p)
{
    int i;

    bzero(p->tabs, cols);
    for (i = 8; i < cols; i += 8)
        p->tabs[i] = 1;
    p->wrap = 1;
    p->origin = 0;
    p->insert = 0;
    p->vbwait = 0;
    p->keypad = 0;
    p->top = 0;
    p->bot = rows - 1;
    p->saved = 0;
    p->LocalAttr = 0;
    p->x = p->y = 0;
    p->state = LIT;
    p->StringType = NONE;
    p->ss = 0;
    p->LocalCharset = G0;
    for (i = G0; i <= G3; i++)
        p->charsets[i] = ASCII;
    return 0;
}

void
WriteString(struct win *wp, char *buf, int len)
{
    int c, intermediate = 0;

    if (!len)
        return;
    curr = wp;
    display = curr->active;
    if (display)
        RemoveStatus(wp);
    do {
        c = *buf++;
        if (c == '\0' || c == '\177')
            continue;
NextChar:
        switch (curr->state) {
        case PRIN:
            switch (c) {
            case '\033':
                curr->state = PRINESC;
                break;
            default:
                PrintChar(c);
            }
            break;
        case PRINESC:
            switch (c) {
            case '[':
                curr->state = PRINCSI;
                break;
            default:
                PrintChar('\033');
                PrintChar(c);
                curr->state = PRIN;
            }
            break;
        case PRINCSI:
            switch (c) {
            case '4':
                curr->state = PRIN4;
                break;
            default:
                PrintChar('\033');
                PrintChar('[');
                PrintChar(c);
                curr->state = PRIN;
            }
            break;
        case PRIN4:
            switch (c) {
            case 'i':
                curr->state = LIT;
                PrintFlush();
                break;
            default:
                PrintChar('\033');
                PrintChar('[');
                PrintChar('4');
                PrintChar(c);
                curr->state = PRIN;
            }
            break;
        case TERM:
            switch (c) {
            case '\\':
                curr->state = LIT;
                *(curr->stringp) = '\0';
                if (curr->StringType == PM && display) {
                    MakeStatus(curr->string, curr);
                    if (status && len > 1) {
                        curr->outlen = len - 1;
                        bcopy(buf, curr->outbuf, curr->outlen);
                        return;
                    }
                }
                break;
            default:
                curr->state = STR;
                AddChar('\033');
                AddChar(c);
            }
            break;
        case STR:
            switch (c) {
            case '\0':
                break;
            case '\033':
                curr->state = TERM;
                break;
            default:
                AddChar(c);
            }
            break;
        case ESC:
            switch (c) {
            case '[':
                curr->NumArgs = 0;
                intermediate = 0;
                bzero((char *)curr->args, MAXARGS * sizeof(int));
                bzero(curr->GotArg, MAXARGS);
                curr->state = CSI;
                break;
            case ']':
                StartString(OSC);
                break;
            case '_':
                StartString(APC);
                break;
            case 'P':
                StartString(DCS);
                break;
            case '^':
                StartString(PM);
                break;
            default:
                if (Special(c))
                    break;
                if (c >= ' ' && c <= '/') {
                    intermediate = intermediate ? -1 : c;
                } else if (c >= '0' && c <= '~') {
                    DoESC(c, intermediate);
                    curr->state = LIT;
                } else {
                    curr->state = LIT;
                    goto NextChar;
                }
            }
            break;
        case CSI:
            switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (curr->NumArgs < MAXARGS) {
                    curr->args[curr->NumArgs] =
                        10 * curr->args[curr->NumArgs] + c - '0';
                    curr->GotArg[curr->NumArgs] = 1;
                }
                break;
            case ';':
            case ':':
                curr->NumArgs++;
                break;
            default:
                if (Special(c))
                    break;
                if (c >= '@' && c <= '~') {
                    curr->NumArgs++;
                    DoCSI(c, intermediate);
                    if (curr->state != PRIN)
                        curr->state = LIT;
                } else if ((c >= ' ' && c <= '/') || (c >= '<' && c <= '?')) {
                    intermediate = intermediate ? -1 : c;
                } else {
                    curr->state = LIT;
                    goto NextChar;
                }
            }
            break;
        default:
            if (!Special(c)) {
                if (c == '\033') {
                    intermediate = 0;
                    curr->state = ESC;
                } else if (c < ' ') {
                    break;
                } else {
                    if (curr->ss)
                        NewCharset(GlobalCharset, curr->charsets[curr->ss]);
                    if (curr->x < cols - 1) {
                        if (curr->insert) {
                            InsertAChar(c);
                        } else {
                            if (display)
                                putchar(c);
                            SetChar(c);
                        }
                        curr->x++;
                    } else if (curr->x == cols - 1) {
                        SetChar(c);
                        if (!(AM && curr->y == curr->bot)) {
                            if (display)
                                putchar(c);
                            Goto(-1, -1, curr->y, curr->x);
                        }
                        curr->x++;
                    } else {
                        if (curr->wrap) {
                            Return();
                            LineFeed();
                            if (curr->insert) {
                                InsertAChar(c);
                            } else {
                                if (display)
                                    putchar(c);
                                SetChar(c);
                            }
                            curr->x = 1;
                        } else
                            curr->x = cols;
                    }
                    if (curr->ss) {
                        NewCharset(curr->charsets[curr->ss], GlobalCharset);
                        curr->ss = 0;
                    }
                }
            }
        }
    } while (--len);
    curr->outlen = 0;
    if (curr->state == PRIN)
        PrintFlush();
}

static int
Special(int c)
{
    switch (c) {
    case '\b':
        BackSpace();
        return 1;
    case '\r':
        Return();
        return 1;
    case '\n':
        LineFeed();
        return 1;
    case '\007':
        PutStr(BL);
        if (!display)
            curr->bell = 1;
        return 1;
    case '\t':
        ForwardTab();
        return 1;
    case '\017':                /* SI */
        MapCharset(G0);
        return 1;
    case '\016':                /* SO */
        MapCharset(G1);
        return 1;
    }
    return 0;
}

void
DoESC(int c, int intermediate)
{
    switch (intermediate) {
    case 0:
        switch (c) {
        case 'E':
            Return();
            LineFeed();
            break;
        case 'D':
            LineFeed();
            break;
        case 'M':
            ReverseLineFeed();
            break;
        case 'H':
            curr->tabs[curr->x] = 1;
            break;
        case '7':
            SaveCursor();
            break;
        case '8':
            RestoreCursor();
            break;
        case 'c':
            ClearScreen();
            Goto(curr->y, curr->x, 0, 0);
            NewRendition(GlobalAttr, 0);
            SetRendition(0);
            NewCharset(GlobalCharset, ASCII);
            GlobalCharset = ASCII;
            if (curr->insert)
                InsertMode(0);
            if (curr->keypad)
                KeypadMode(0);
            if (CS)
                PutStr(tgoto(CS, rows - 1, 0));
            ResetScreen(curr);
            break;
        case '=':
            KeypadMode(1);
            curr->keypad = 1;
            break;
        case '>':
            KeypadMode(0);
            curr->keypad = 0;
            break;
        case 'n':               /* LS2 */
            MapCharset(G2);
            break;
        case 'o':               /* LS3 */
            MapCharset(G3);
            break;
        case 'N':               /* SS2 */
            if (GlobalCharset == curr->charsets[G2])
                curr->ss = 0;
            else
                curr->ss = G2;
            break;
        case 'O':               /* SS3 */
            if (GlobalCharset == curr->charsets[G3])
                curr->ss = 0;
            else
                curr->ss = G3;
            break;
        }
        break;
    case '#':
        switch (c) {
        case '8':
            FillWithEs();
            break;
        }
        break;
    case '(':
        DesignateCharset(c, G0);
        break;
    case ')':
        DesignateCharset(c, G1);
        break;
    case '*':
        DesignateCharset(c, G2);
        break;
    case '+':
        DesignateCharset(c, G3);
        break;
    }
}

static int
DoCSI(int c, int intermediate)
{
    int i, a1 = curr->args[0], a2 = curr->args[1];

    if (curr->NumArgs >= MAXARGS)
        curr->NumArgs = MAXARGS;
    for (i = 0; i < curr->NumArgs; ++i)
        if (curr->args[i] == 0)
            curr->GotArg[i] = 0;
    switch (intermediate) {
    case 0:
        switch (c) {
        case 'H':
        case 'f':
            if (!curr->GotArg[0])
                a1 = 1;
            if (!curr->GotArg[1])
                a2 = 1;
            if (curr->origin)
                a1 += curr->top;
            if (a1 < 1)
                a1 = 1;
            if (a1 > rows)
                a1 = rows;
            if (a2 < 1)
                a2 = 1;
            if (a2 > cols)
                a2 = cols;
            a1--;
            a2--;
            Goto(curr->y, curr->x, a1, a2);
            curr->y = a1;
            curr->x = a2;
            break;
        case 'J':
            if (!curr->GotArg[0] || a1 < 0 || a1 > 2)
                a1 = 0;
            switch (a1) {
            case 0:
                ClearToEOS();
                break;
            case 1:
                ClearFromBOS();
                break;
            case 2:
                ClearScreen();
                Goto(0, 0, curr->y, curr->x);
                break;
            }
            break;
        case 'K':
            if (!curr->GotArg[0] || a1 < 0 || a1 > 2)
                a1 %= 3;
            switch (a1) {
            case 0:
                ClearToEOL();
                break;
            case 1:
                ClearFromBOL();
                break;
            case 2:
                ClearLine();
                break;
            }
            break;
        case 'A':
            CursorUp(curr->GotArg[0] ? a1 : 1);
            break;
        case 'B':
            CursorDown(curr->GotArg[0] ? a1 : 1);
            break;
        case 'C':
            CursorRight(curr->GotArg[0] ? a1 : 1);
            break;
        case 'D':
            CursorLeft(curr->GotArg[0] ? a1 : 1);
            break;
        case 'm':
            SelectRendition();
            break;
        case 'g':
            if (!curr->GotArg[0] || a1 == 0)
                curr->tabs[curr->x] = 0;
            else if (a1 == 3)
                bzero(curr->tabs, cols);
            break;
        case 'r':
            if (!CS)
                break;
            if (!curr->GotArg[0])
                a1 = 1;
            if (!curr->GotArg[1])
                a2 = rows;
            if (a1 < 1 || a2 > rows || a1 >= a2)
                break;
            curr->top = a1 - 1;
            curr->bot = a2 - 1;
            PutStr(tgoto(CS, curr->bot, curr->top));
            if (curr->origin) {
                Goto(-1, -1, curr->top, 0);
                curr->y = curr->top;
                curr->x = 0;
            } else {
                Goto(-1, -1, 0, 0);
                curr->y = curr->x = 0;
            }
            break;
        case 'I':
            if (!curr->GotArg[0])
                a1 = 1;
            while (a1--)
                ForwardTab();
            break;
        case 'Z':
            if (!curr->GotArg[0])
                a1 = 1;
            while (a1--)
                BackwardTab();
            break;
        case 'L':
            InsertLine(curr->GotArg[0] ? a1 : 1);
            break;
        case 'M':
            DeleteLine(curr->GotArg[0] ? a1 : 1);
            break;
        case 'P':
            DeleteChar(curr->GotArg[0] ? a1 : 1);
            break;
        case '@':
            InsertChar(curr->GotArg[0] ? a1 : 1);
            break;
        case 'h':
            SetMode(1);
            break;
        case 'l':
            SetMode(0);
            break;
        case 'i':
            if (PO && curr->GotArg[0] && a1 == 5) {
                curr->stringp = curr->string;
                curr->state = PRIN;
            }
            break;
        }
        break;
    case '?':
        if (c != 'h' && c != 'l')
            break;
        if (!curr->GotArg[0])
            break;
        i = (c == 'h');
        if (a1 == 5) {
            if (i) {
                curr->vbwait = 1;
            } else {
                if (curr->vbwait)
                    PutStr(VB);
                curr->vbwait = 0;
            }
        } else if (a1 == 6) {
            curr->origin = i;
            if (curr->origin) {
                Goto(curr->y, curr->x, curr->top, 0);
                curr->y = curr->top;
                curr->x = 0;
            } else {
                Goto(curr->y, curr->x, 0, 0);
                curr->y = curr->x = 0;
            }
        } else if (a1 == 7) {
            curr->wrap = i;
        }
        break;
    }
    return 0;
}

static int
PutChar(int c)
{
    putchar(c);
    return 0;
}

static int
PutStr(char *s)
{
    if (display && s)
        tputs(s, 1, PutChar);
    return 0;
}

static int
CPutStr(char *s, int c)
{
    if (display && s)
        tputs(tgoto(s, 0, c), 1, PutChar);      /* XXX */
    return 0;
}

static int
SetChar(int c)
{
    struct win *p = curr;

    p->image[p->y][p->x] = c;
    p->attr[p->y][p->x] = p->LocalAttr;
    p->font[p->y][p->x] = p->charsets[p->ss ? p->ss : p->LocalCharset];
    return 0;
}

static int
StartString(enum string_t type)
{
    curr->StringType = type;
    curr->stringp = curr->string;
    curr->state = STR;
    return 0;
}

static int
AddChar(int c)
{
    if (curr->stringp >= curr->string + MAXSTR - 1)
        curr->state = LIT;
    else
        *(curr->stringp)++ = c;
    return 0;
}

static int
PrintChar(int c)
{
    if (curr->stringp >= curr->string + MAXSTR - 1)
        PrintFlush();
    else
        *(curr->stringp)++ = c;
    return 0;
}

static int
PrintFlush(void)
{
    if (curr->stringp > curr->string) {
        tputs(PO, 1, PutChar);
        fflush(stdout);
        write(1, curr->string, curr->stringp - curr->string);
        tputs(PF, 1, PutChar);
        fflush(stdout);
        curr->stringp = curr->string;
    }
    return 0;
}

/*
 * Insert mode is a toggle on some terminals, so we need this hack:
 */
static int
InsertMode(int on)
{
    if (on) {
        if (!insert)
            PutStr(IM);
    } else if (insert)
        PutStr(EI);
    insert = on;
    return 0;
}

/*
 * ...and maybe keypad application mode is a toggle, too:
 */
static int
KeypadMode(int on)
{
    if (on) {
        if (!keypad)
            PutStr(KS);
    } else if (keypad)
        PutStr(KE);
    keypad = on;
    return 0;
}

static int
DesignateCharset(int c, int n)
{
    curr->ss = 0;
    if (c == 'B')
        c = ASCII;
    if (curr->charsets[n] != c) {
        curr->charsets[n] = c;
        if (curr->LocalCharset == n) {
            NewCharset(GlobalCharset, c);
            GlobalCharset = c;
        }
    }
    return 0;
}

static int
MapCharset(int n)
{
    curr->ss = 0;
    if (curr->LocalCharset != n) {
        curr->LocalCharset = n;
        NewCharset(GlobalCharset, curr->charsets[n]);
        GlobalCharset = curr->charsets[n];
    }
    return 0;
}

static void
NewCharset(int old, int new)
{
    char buf[8];

    if (old == new)
        return;
    if (ISO2022) {
        sprintf(buf, "\033(%c", new == ASCII ? 'B' : new);
        PutStr(buf);
    }
}

static int
SaveCursor(void)
{
    curr->saved = 1;
    curr->Saved_x = curr->x;
    curr->Saved_y = curr->y;
    curr->SavedLocalAttr = curr->LocalAttr;
    curr->SavedLocalCharset = curr->LocalCharset;
    bcopy((char *)curr->charsets, (char *)curr->SavedCharsets,
          4 * sizeof(int));
    return 0;
}

static int
RestoreCursor(void)
{
    if (curr->saved) {
        curr->LocalAttr = curr->SavedLocalAttr;
        NewRendition(GlobalAttr, curr->LocalAttr);
        GlobalAttr = curr->LocalAttr;
        bcopy((char *)curr->SavedCharsets, (char *)curr->charsets,
              4 * sizeof(int));
        curr->LocalCharset = curr->SavedLocalCharset;
        NewCharset(GlobalCharset, curr->charsets[curr->LocalCharset]);
        GlobalCharset = curr->charsets[curr->LocalCharset];
        Goto(curr->y, curr->x, curr->Saved_y, curr->Saved_x);
        curr->x = curr->Saved_x;
        curr->y = curr->Saved_y;
    }
    return 0;
}

static int
CountChars(int c)
{
    StrCost++;
    return 0;
}

static int
CalcCost(char *s)
{
    if (s) {
        StrCost = 0;
        tputs(s, 1, CountChars);
        return StrCost;
    } else
        return EXPENSIVE;
}

static void
Goto(int y1, int x1, int y2, int x2)
{
    int dy, dx;
    int cost = 0;
    char *s;
    int CMcost, n, m;
    enum move_t xm = M_NONE, ym = M_NONE;

    if (!display)
        return;
    if (x1 == cols || x2 == cols) {
        if (x2 == cols)
            --x2;
        goto DoCM;
    }
    dx = x2 - x1;
    dy = y2 - y1;
    if (dy == 0 && dx == 0)
        return;
    if (y1 == -1 || x1 == -1 || y2 >= curr->bot || y2 <= curr->top) {
DoCM:
        PutStr(tgoto(CM, x2, y2));
        return;
    }
    CMcost = CalcCost(tgoto(CM, x2, y2));
    if (dx > 0) {
        if ((n = RewriteCost(y1, x1, x2)) < (m = dx * NDcost)) {
            cost = n;
            xm = M_RW;
        } else {
            cost = m;
            xm = M_RI;
        }
    } else if (dx < 0) {
        cost = -dx * LEcost;
        xm = M_LE;
    }
    if (dx && (n = RewriteCost(y1, 0, x2) + CRcost) < cost) {
        cost = n;
        xm = M_CR;
    }
    if (cost >= CMcost)
        goto DoCM;
    if (dy > 0) {
        cost += dy * DOcost;
        ym = M_DO;
    } else if (dy < 0) {
        cost += -dy * UPcost;
        ym = M_UP;
    }
    if (cost >= CMcost)
        goto DoCM;
    if (xm != M_NONE) {
        if (xm == M_LE || xm == M_RI) {
            if (xm == M_LE) {
                s = BC;
                dx = -dx;
            } else
                s = ND;
            while (dx-- > 0)
                PutStr(s);
        } else {
            if (xm == M_CR) {
                PutStr(CR);
                x1 = 0;
            }
            if (x1 < x2) {
                if (curr->insert)
                    InsertMode(0);
                for (s = curr->image[y1] + x1; x1 < x2; x1++, s++)
                    putchar(*s);
                if (curr->insert)
                    InsertMode(1);
            }
        }
    }
    if (ym != M_NONE) {
        if (ym == M_UP) {
            s = UP;
            dy = -dy;
        } else
            s = DO;
        while (dy-- > 0)
            PutStr(s);
    }
}

static int
RewriteCost(int y, int x1, int x2)
{
    int cost, dx;
    char *p = curr->attr[y] + x1, *f = curr->font[y] + x1;

    if (AM && y == rows - 1 && x2 == cols - 1)
        return EXPENSIVE;
    cost = dx = x2 - x1;
    if (dx == 0)
        return 0;
    if (curr->insert)
        cost += EIcost + IMcost;
    do {
        if (*p++ != GlobalAttr || *f++ != GlobalCharset)
            return EXPENSIVE;
    } while (--dx);
    return cost;
}

static int
BackSpace(void)
{
    if (curr->x > 0) {
        if (curr->x < cols) {
            if (BC)
                PutStr(BC);
            else
                Goto(curr->y, curr->x, curr->y, curr->x - 1);
        }
        curr->x--;
    }
    return 0;
}

static int
Return(void)
{
    if (curr->x > 0) {
        curr->x = 0;
        PutStr(CR);
    }
    return 0;
}

static int
LineFeed(void)
{
    if (curr->y == curr->bot) {
        ScrollUpMap(curr->image);
        ScrollUpMap(curr->attr);
        ScrollUpMap(curr->font);
    } else if (curr->y < rows - 1) {
        curr->y++;
    }
    PutStr(NL);
    return 0;
}

static int
ReverseLineFeed(void)
{
    if (curr->y == curr->top) {
        ScrollDownMap(curr->image);
        ScrollDownMap(curr->attr);
        ScrollDownMap(curr->font);
        if (SR) {
            PutStr(SR);
        } else if (AL) {
            Goto(curr->top, curr->x, curr->top, 0);
            PutStr(AL);
            Goto(curr->top, 0, curr->top, curr->x);
        } else
            Redisplay();
    } else if (curr->y > 0) {
        CursorUp(1);
    }
    return 0;
}

static void
InsertAChar(int c)
{
    int y = curr->y, x = curr->x;

    if (x == cols)
        x--;
    bcopy(curr->image[y], OldImage, cols);
    bcopy(curr->attr[y], OldAttr, cols);
    bcopy(curr->font[y], OldFont, cols);
    bcopy(curr->image[y] + x, curr->image[y] + x + 1, cols - x - 1);
    bcopy(curr->attr[y] + x, curr->attr[y] + x + 1, cols - x - 1);
    bcopy(curr->font[y] + x, curr->font[y] + x + 1, cols - x - 1);
    SetChar(c);
    if (!display)
        return;
    if (IC || IM) {
        if (!curr->insert)
            InsertMode(1);
        PutStr(IC);
        putchar(c);
        if (!curr->insert)
            InsertMode(0);
    } else {
        RedisplayLine(OldImage, OldAttr, OldFont, y, x, cols - 1);
        ++x;
        Goto(y, last_x, y, x);
    }
}

static void
InsertChar(int n)
{
    int i, y = curr->y, x = curr->x;

    if (x == cols)
        return;
    bcopy(curr->image[y], OldImage, cols);
    bcopy(curr->attr[y], OldAttr, cols);
    bcopy(curr->font[y], OldFont, cols);
    if (n > cols - x)
        n = cols - x;
    bcopy(curr->image[y] + x, curr->image[y] + x + n, cols - x - n);
    bcopy(curr->attr[y] + x, curr->attr[y] + x + n, cols - x - n);
    bcopy(curr->font[y] + x, curr->font[y] + x + n, cols - x - n);
    ClearInLine(0, y, x, x + n - 1);
    if (!display)
        return;
    if (IC || IM) {
        if (!curr->insert)
            InsertMode(1);
        for (i = n; i; i--) {
            PutStr(IC);
            putchar(' ');
        }
        if (!curr->insert)
            InsertMode(0);
        Goto(y, x + n, y, x);
    } else {
        RedisplayLine(OldImage, OldAttr, OldFont, y, x, cols - 1);
        Goto(y, last_x, y, x);
    }
}

static void
DeleteChar(int n)
{
    int i, y = curr->y, x = curr->x;

    if (x == cols)
        return;
    bcopy(curr->image[y], OldImage, cols);
    bcopy(curr->attr[y], OldAttr, cols);
    bcopy(curr->font[y], OldFont, cols);
    if (n > cols - x)
        n = cols - x;
    bcopy(curr->image[y] + x + n, curr->image[y] + x, cols - x - n);
    bcopy(curr->attr[y] + x + n, curr->attr[y] + x, cols - x - n);
    bcopy(curr->font[y] + x + n, curr->font[y] + x, cols - x - n);
    ClearInLine(0, y, cols - n, cols - 1);
    if (!display)
        return;
    if (CDC && !(n == 1 && DC)) {
        CPutStr(CDC, n);
    } else if (DC) {
        for (i = n; i; i--)
            PutStr(DC);
    } else {
        RedisplayLine(OldImage, OldAttr, OldFont, y, x, cols - 1);
        Goto(y, last_x, y, x);
    }
}

static int
DeleteLine(int n)
{
    int i, old = curr->top;

    if (n > curr->bot - curr->y + 1)
        n = curr->bot - curr->y + 1;
    curr->top = curr->y;
    for (i = n; i; i--) {
        ScrollUpMap(curr->image);
        ScrollUpMap(curr->attr);
        ScrollUpMap(curr->font);
    }
    if (DL || CDL) {
        Goto(curr->y, curr->x, curr->y, 0);
        if (CDL && !(n == 1 && DL)) {
            CPutStr(CDL, n);
        } else {
            for (i = n; i; i--)
                PutStr(DL);
        }
        Goto(curr->y, 0, curr->y, curr->x);
    } else if (CS) {
        PutStr(tgoto(CS, curr->bot, curr->top));
        Goto(-1, -1, curr->bot, 0);
        for (i = n; i; i--)
            PutStr(SF);
        PutStr(tgoto(CS, curr->bot, old));
        Goto(-1, -1, curr->y, curr->x);
    } else
        Redisplay();
    curr->top = old;
    return 0;
}

static int
InsertLine(int n)
{
    int i, old = curr->top;

    if (n > curr->bot - curr->y + 1)
        n = curr->bot - curr->y + 1;
    curr->top = curr->y;
    for (i = n; i; i--) {
        ScrollDownMap(curr->image);
        ScrollDownMap(curr->attr);
        ScrollDownMap(curr->font);
    }
    if (AL || CAL) {
        Goto(curr->y, curr->x, curr->y, 0);
        if (CAL && !(n == 1 && AL)) {
            CPutStr(CAL, n);
        } else {
            for (i = n; i; i--)
                PutStr(AL);
        }
        Goto(curr->y, 0, curr->y, curr->x);
    } else if (CS && SR) {
        PutStr(tgoto(CS, curr->bot, curr->top));
        Goto(-1, -1, curr->y, 0);
        for (i = n; i; i--)
            PutStr(SR);
        PutStr(tgoto(CS, curr->bot, old));
        Goto(-1, -1, curr->y, curr->x);
    } else
        Redisplay();
    curr->top = old;
    return 0;
}

static int
ScrollUpMap(char **pp)
{
    char *tmp = pp[curr->top];

    bcopy((char *)(pp + curr->top + 1), (char *)(pp + curr->top),
          (curr->bot - curr->top) * sizeof(char *));
    if (pp == curr->image)
        bclear(tmp, cols);
    else
        bzero(tmp, cols);
    pp[curr->bot] = tmp;
    return 0;
}

static int
ScrollDownMap(char **pp)
{
    char *tmp = pp[curr->bot];

    bcopy((char *)(pp + curr->top), (char *)(pp + curr->top + 1),
          (curr->bot - curr->top) * sizeof(char *));
    if (pp == curr->image)
        bclear(tmp, cols);
    else
        bzero(tmp, cols);
    pp[curr->top] = tmp;
    return 0;
}

static int
ForwardTab(void)
{
    int x = curr->x;

    if (curr->tabs[x] && x < cols - 1)
        ++x;
    while (x < cols - 1 && !curr->tabs[x])
        x++;
    Goto(curr->y, curr->x, curr->y, x);
    curr->x = x;
    return 0;
}

static int
BackwardTab(void)
{
    int x = curr->x;

    if (curr->tabs[x] && x > 0)
        x--;
    while (x > 0 && !curr->tabs[x])
        x--;
    Goto(curr->y, curr->x, curr->y, x);
    curr->x = x;
    return 0;
}

static int
ClearScreen(void)
{
    int i;

    PutStr(CL);
    for (i = 0; i < rows; ++i) {
        bclear(curr->image[i], cols);
        bzero(curr->attr[i], cols);
        bzero(curr->font[i], cols);
    }
    return 0;
}

static int
ClearFromBOS(void)
{
    int n, y = curr->y, x = curr->x;

    for (n = 0; n < y; ++n)
        ClearInLine(1, n, 0, cols - 1);
    ClearInLine(1, y, 0, x);
    Goto(curr->y, curr->x, y, x);
    curr->y = y;
    curr->x = x;
    return 0;
}

static int
ClearToEOS(void)
{
    int n, y = curr->y, x = curr->x;

    if (CD)
        PutStr(CD);
    ClearInLine(!CD, y, x, cols - 1);
    for (n = y + 1; n < rows; n++)
        ClearInLine(!CD, n, 0, cols - 1);
    Goto(curr->y, curr->x, y, x);
    curr->y = y;
    curr->x = x;
    return 0;
}

static int
ClearLine(void)
{
    int y = curr->y, x = curr->x;

    ClearInLine(1, y, 0, cols - 1);
    Goto(curr->y, curr->x, y, x);
    curr->y = y;
    curr->x = x;
    return 0;
}

static int
ClearToEOL(void)
{
    int y = curr->y, x = curr->x;

    ClearInLine(1, y, x, cols - 1);
    Goto(curr->y, curr->x, y, x);
    curr->y = y;
    curr->x = x;
    return 0;
}

static void
ClearFromBOL(void)
{
    int y = curr->y, x = curr->x;

    ClearInLine(1, y, 0, x);
    Goto(curr->y, curr->x, y, x);
    curr->y = y;
    curr->x = x;
}

static void
ClearInLine(int displ, int y, int x1, int x2)
{
    int i, n;

    if (x1 == cols)
        x1--;
    if (x2 == cols)
        x2--;
    if ((n = (x2 - x1 + 1))) {
        bclear(curr->image[y] + x1, n);
        bzero(curr->attr[y] + x1, n);
        bzero(curr->font[y] + x1, n);
        if (displ && display) {
            if (x2 == cols - 1 && CE) {
                Goto(curr->y, curr->x, y, x1);
                curr->y = y;
                curr->x = x1;
                PutStr(CE);
                return;
            }
            if (y == rows - 1 && AM)
                --n;
            if (n == 0)
                return;
            SaveAttr(0);
            Goto(curr->y, curr->x, y, x1);
            for (i = n; i > 0; i--)
                putchar(' ');
            curr->y = y;
            curr->x = x1 + n;
            RestoreAttr(0);
        }
    }
}

static void
CursorRight(int n)
{
    int x = curr->x;

    if (x == cols)
        return;
    if ((curr->x += n) >= cols)
        curr->x = cols - 1;
    Goto(curr->y, x, curr->y, curr->x);
}

static void
CursorUp(int n)
{
    int y = curr->y;

    if ((curr->y -= n) < curr->top)
        curr->y = curr->top;
    Goto(y, curr->x, curr->y, curr->x);
}

static void
CursorDown(int n)
{
    int y = curr->y;

    if ((curr->y += n) > curr->bot)
        curr->y = curr->bot;
    Goto(y, curr->x, curr->y, curr->x);
}

static void
CursorLeft(int n)
{
    int x = curr->x;

    if ((curr->x -= n) < 0)
        curr->x = 0;
    Goto(curr->y, x, curr->y, curr->x);
}

static int
SetMode(int on)
{
    int i;

    for (i = 0; i < curr->NumArgs; ++i) {
        switch (curr->args[i]) {
        case 4:
            curr->insert = on;
            InsertMode(on);
            break;
        }
    }
    return 0;
}

static int
SelectRendition(void)
{
    int i, old = GlobalAttr;

    if (curr->NumArgs == 0)
        SetRendition(0);
    else
        for (i = 0; i < curr->NumArgs; ++i)
            SetRendition(curr->args[i]);
    NewRendition(old, GlobalAttr);
    return 0;
}

static int
SetRendition(int n)
{
    switch (n) {
    case 0:
        GlobalAttr = 0;
        break;
    case 1:
        GlobalAttr |= A_BD;
        break;
    case 2:
        GlobalAttr |= A_DI;
        break;
    case 3:
        GlobalAttr |= A_SO;
        break;
    case 4:
        GlobalAttr |= A_US;
        break;
    case 5:
        GlobalAttr |= A_BL;
        break;
    case 7:
        GlobalAttr |= A_RV;
        break;
    case 22:
        GlobalAttr &= ~(A_BD | A_SO | A_DI);
        break;
    case 23:
        GlobalAttr &= ~A_SO;
        break;
    case 24:
        GlobalAttr &= ~A_US;
        break;
    case 25:
        GlobalAttr &= ~A_BL;
        break;
    case 27:
        GlobalAttr &= ~A_RV;
        break;
    }
    curr->LocalAttr = GlobalAttr;
    return 0;
}

static void
NewRendition(int old, int new)
{
    int i;

    if (old == new)
        return;
    for (i = 1; i <= A_MAX; i <<= 1) {
        if ((old & i) && !(new & i)) {
            PutStr(UE);
            PutStr(SE);
            PutStr(ME);
            if (new & A_US)
                PutStr(US);
            if (new & A_SO)
                PutStr(SO);
            if (new & A_BL)
                PutStr(MB);
            if (new & A_BD)
                PutStr(MD);
            if (new & A_DI)
                PutStr(MH);
            if (new & A_RV)
                PutStr(MR);
            return;
        }
    }
    if ((new & A_US) && !(old & A_US))
        PutStr(US);
    if ((new & A_SO) && !(old & A_SO))
        PutStr(SO);
    if ((new & A_BL) && !(old & A_BL))
        PutStr(MB);
    if ((new & A_BD) && !(old & A_BD))
        PutStr(MD);
    if ((new & A_DI) && !(old & A_DI))
        PutStr(MH);
    if ((new & A_RV) && !(old & A_RV))
        PutStr(MR);
}

static int
SaveAttr(int newattr)
{
    NewRendition(GlobalAttr, newattr);
    NewCharset(GlobalCharset, ASCII);
    if (curr->insert)
        InsertMode(0);
    return 0;
}

static int
RestoreAttr(int oldattr)
{
    NewRendition(oldattr, GlobalAttr);
    NewCharset(ASCII, GlobalCharset);
    if (curr->insert)
        InsertMode(1);
    return 0;
}

static int
FillWithEs(void)
{
    int i;
    char *p, *ep;

    curr->y = curr->x = 0;
    SaveAttr(0);
    for (i = 0; i < rows; ++i) {
        bzero(curr->attr[i], cols);
        bzero(curr->font[i], cols);
        p = curr->image[i];
        ep = p + cols;
        for (; p < ep; ++p)
            *p = 'E';
    }
    RestoreAttr(0);
    Redisplay();
    return 0;
}

static int
Redisplay(void)
{
    int i;

    PutStr(CL);
    TmpAttr = GlobalAttr;
    TmpCharset = GlobalCharset;
    InsertMode(0);
    last_x = last_y = 0;
    for (i = 0; i < rows; ++i)
        DisplayLine(blank, null, null, curr->image[i], curr->attr[i],
                    curr->font[i], i, 0, cols - 1);
    if (curr->insert)
        InsertMode(1);
    NewRendition(TmpAttr, GlobalAttr);
    NewCharset(TmpCharset, GlobalCharset);
    Goto(last_y, last_x, curr->y, curr->x);
    return 0;
}

static void
DisplayLine(char *os, char *oa, char *of, char *s, char *as, char *fs, int y, int from, int to)
{
    int i, x, a, f;

    if (to == cols)
        --to;
    if (AM && y == rows - 1 && to == cols - 1)
        --to;
    a = TmpAttr;
    f = TmpCharset;
    for (x = i = from; i <= to; ++i, ++x) {
        if (s[i] == os[i] && as[i] == oa[i] && as[i] == a
            && of[i] == fs[i] && fs[i] == f)
            continue;
        Goto(last_y, last_x, y, x);
        last_y = y;
        last_x = x;
        if ((a = as[i]) != TmpAttr) {
            NewRendition(TmpAttr, a);
            TmpAttr = a;
        }
        if ((f = fs[i]) != TmpCharset) {
            NewCharset(TmpCharset, f);
            TmpCharset = f;
        }
        putchar(s[i]);
        last_x++;
    }
}

static void
RedisplayLine(char *os, char *oa, char *of, int y, int from, int to)
{
    if (curr->insert)
        InsertMode(0);
    NewRendition(GlobalAttr, 0);
    TmpAttr = 0;
    NewCharset(GlobalCharset, ASCII);
    TmpCharset = ASCII;
    last_y = y;
    last_x = from;
    DisplayLine(os, oa, of, curr->image[y], curr->attr[y],
                curr->font[y], y, from, to);
    NewRendition(TmpAttr, GlobalAttr);
    NewCharset(TmpCharset, GlobalCharset);
    if (curr->insert)
        InsertMode(1);
}

static int
MakeBlankLine(char *p, int n)
{
    do
        *p++ = ' ';
    while (--n);
    return 0;
}

int
MakeStatus(char *msg, struct win *wp)
{
    struct win *ocurr = curr;
    int odisplay = display;
    char *s, *t;
    int max = AM ? cols - 1 : cols;

    for (s = t = msg; *s && t - msg < max; ++s)
        if (*s >= ' ' && *s <= '~')
            *t++ = *s;
    *t = '\0';
    curr = wp;
    display = 1;
    if (status) {
        if (time((time_t *) 0) - TimeDisplayed < 2)
            sleep(1);
        RemoveStatus(wp);
    }
    if (t > msg) {
        status = 1;
        StatLen = t - msg;
        Goto(curr->y, curr->x, rows - 1, 0);
        SaveAttr(A_SO);
        printf("%s", msg);
        RestoreAttr(A_SO);
        fflush(stdout);
        time(&TimeDisplayed);
    }
    curr = ocurr;
    display = odisplay;
    return 0;
}

void
RemoveStatus(struct win *p)
{
    struct win *ocurr = curr;
    int odisplay = display;

    if (!status)
        return;
    status = 0;
    curr = p;
    display = 1;
    Goto(-1, -1, rows - 1, 0);
    RedisplayLine(null, null, null, rows - 1, 0, StatLen);
    Goto(rows - 1, last_x, curr->y, curr->x);
    curr = ocurr;
    display = odisplay;
}
