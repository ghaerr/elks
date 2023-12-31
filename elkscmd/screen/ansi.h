/*
 * Header file for ansi.c
 *
 * with function declarations
 *
 */

int InitTerm();
int FinitTerm();
static int AddCap(char *s);
char *MakeTermcap(int aflag);
static void DisplayLine(char *os, char *oa, char *of, char *s, char *as, char *fs, int y, int from, int to);
static int MakeString(char *cap, char *buf, char *s);
static int Special(int c);
static int DoCSI(int c, int intermediate);
static int PutChar(int c);
static int PutStr(char *s);
static int CPutStr(char *s, int c);
static int SetChar(int c);
static int StartString(enum string_t type);
static int AddChar(int c);
static int PrintChar(int c);
static int PrintFlush();
static int InsertMode(int on);
static int KeypadMode(int on);
static int DesignateCharset(int c, int n);
static int MapCharset(int n);
static void NewCharset(int old, int new);
static int SaveCursor();
static int RestoreCursor();
static void CountChars(int c);
static int CalcCost(char *s);
static void Goto(int y1, int x1, int y2, int x2);
static int RewriteCost(int y, int x1, int x2);
static int BackSpace();
static int Return();
static int LineFeed();
static int ReverseLineFeed();
static void InsertAChar(int c);
static void InsertChar(int n);
static void DeleteChar(int n);
static int DeleteLine(int n);
static int InsertLine(int n);
static int ScrollUpMap(char **pp);
static int ScrollDownMap(char **pp);
static int ForwardTab();
static int BackwardTab();
static int ClearScreen();
static int ClearFromBOS();
static int ClearToEOS();
static int ClearLine();
static int ClearToEOL();
static void ClearFromBOL();
static void ClearInLine(int displ, int y, int x1, int x2);
static void CursorRight(int n);
static void CursorUp(int n);
static void CursorDown(int n);
static void CursorLeft(int n);
static int SetMode(int on);
static int SelectRendition();
static int SetRendition(int n);
static void NewRendition(int old, int new);
static int SaveAttr(int newattr);
static int RestoreAttr(int oldattr);
static int FillWithEs();
static int Redisplay();
static void RedisplayLine(char *os, char *oa, char *of, int y, int from, int to);
static int MakeBlankLine(char *p, int n);

int Activate(struct win *wp);
int ResetScreen(struct win *p);
void WriteString(struct win *wp, char *buf, int len);
void DoESC(int c, int intermediate);
int MakeStatus(char *msg, struct win *wp);
void RemoveStatus(struct win *p);
