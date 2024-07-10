/* curses.h workaround - Greg Haerr*/

#define KEY_BACKSPACE   kBackSpace
#define KEY_LEFT        kLeftArrow
#define KEY_ENTER       '\r'
#define KEY_RIGHT       kRightArrow
#define KEY_UP          kUpArrow
#define KEY_DOWN        kDownArrow
#define KEY_HOME        kHome
#define KEY_END         kEnd
#define KEY_NPAGE       kPageDown
#define KEY_PPAGE       kPageUp
#define KEY_RESIZE      kHome       /* dup */

#define A_NORMAL        0x0000
#define A_BLINK         0x0100
#define A_BOLD          0x0200

#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHTGRAY     7
#define COLOR_DARKGRAY      8
#define COLOR_LIGHTBLUE     9
#define COLOR_LIGHTGREEN   10
#define COLOR_LIGHTCYAN    11
#define COLOR_LIGHTRED     12
#define COLOR_LIGHTMAGENTA 13
#define COLOR_YELLOW       14
#define COLOR_WHITE        15

#define COLOR_PAIR(n)   (n)

#define TRUE            1
#define FALSE           0

#define OK              0
#define ERR             (-1)

extern int LINES;
extern int COLS;
extern void *stdscr;

void start_color();
int use_default_colors();
void init_pair(int ndx, int fg, int bg);
void *initscr();
void endwin();
int has_colors();
void erase();
void nodelay();
#define cbreak()
#define noecho()
#define echo()
#define nonl()
#define intrflush(w,flag)
#define keypad(scr,flag)
#define timeout(t)
#define leaveok(w,f)
#define scrollok(w,f)
#define clear()             erase()
#define werase(w)           erase()
#define wgetch(w)           getch()

void curs_set(int cursoron);
void move(int y, int x);
void clrnl(void);
void clrtoeos(void);
void clrtoeol(void);
void printw(char *, ...);
void waitformouse(void);
int getch();
void wgetnstr(void *, char *, int);
void attron(int a);
void attroff(int a);

void refresh();
void mvcur(int,int,int,int);
int mvaddch(int,int,int);
int addch(int);

/* structure contents unused, only typedefs needed */
typedef struct screen {
    int unused;
} SCREEN;

typedef struct window {
    int unused;
} WINDOW;

typedef int chtype;

/* partially implemented functions for ttyclock */
void mvwaddch(WINDOW *w, int y, int x, int ch);
void mvwaddstr(WINDOW *w, int y, int x, char *str);
void mvwprintw(WINDOW *w, int y, int x, char *fmt, ...);
void mvwin(WINDOW *w, int y, int x);
void wrefresh(WINDOW *w);
#define newterm(type,ofd,ifd)   NULL
#define newwin(l,c,y,x)         NULL
#define delscreen(s)
#define set_term(s)
#define clearok(w,flag)
#define box(w,y,x)
#define wborder(w,a,b,c,d,e,f,g,h)
#define wresize(win,h,w)
void wattron(WINDOW *w, int a);
void wattroff(WINDOW *w, int a);
void wbkgdset(WINDOW *w, int a);
