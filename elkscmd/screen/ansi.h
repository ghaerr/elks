/* Header file for ansi.c
 * 
 *  with function declarations
 * 
*/

int InitTerm(void);
int FinitTerm(void);
static AddCap(char *s);
char *MakeTermcap(int aflag);
static MakeString(char *cap, char *buf, register char *s);
int Activate(struct win *wp);
int ResetScreen(register struct win *p);
void WriteString(struct win *wp, register char *buf, int len);
static Special(register c);
void DoESC(int c, int intermediate);
static DoCSI(int c, int intermediate);
static PutChar(int c);
static PutStr(char *s);
static CPutStr(char *s, int c);
static SetChar(register c);
static StartString(enum string_t type);
static AddChar(int c);
static PrintChar(int c);
static PrintFlush(void);
static InsertMode(int on);
static KeypadMode(int on);
static DesignateCharset(int c, int n);
static MapCharset(int n);
static void NewCharset(int old, int new);
static SaveCursor(void);
static RestoreCursor(void);
static void CountChars(int c);
static CalcCost(register char *s);
static void Goto(int y1, int x1, int y2, int x2);
static RewriteCost(int y, int x1, int x2);
static BackSpace(void);
static Return(void);
static LineFeed(void);
static ReverseLineFeed(void);
static void InsertAChar(int c);
static void InsertChar(int n);
static void DeleteChar(int n);
static DeleteLine(int n);
static InsertLine(int n);
static ScrollUpMap(char **pp);
static ScrollDownMap(char **pp);
static ForwardTab(void);
static BackwardTab(void);
static ClearScreen(void);
static ClearFromBOS(void);
static ClearToEOS(void);
static ClearLine(void);
static ClearToEOL(void);
static void ClearFromBOL(void);
static void ClearInLine(int displ, int y, int x1, int x2);
static void CursorRight(register n);
static void CursorUp(register n);
static void CursorDown(register n);
static void CursorLeft(register n);
static SetMode(int on);
static SelectRendition(void);
static SetRendition(register n);
static void NewRendition(register old, register new);
static SaveAttr(int newattr);
static RestoreAttr(int oldattr);
static FillWithEs(void);
static Redisplay(void);
static MakeBlankLine(register char *p, register n);
int MakeStatus(char *msg, struct win *wp);
void RemoveStatus(struct win *p);

