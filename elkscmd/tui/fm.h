/* See LICENSE file for copyright and license details. */
#define CWD   ""
#define CURSR " > "
#define EMPTY "   "

#define PAGER "more"

/* See curs_attr(3) for valid video attributes */
#define CURSR_ATTR A_NORMAL
#define DIR_ATTR   A_NORMAL | COLOR_PAIR(4)
#define LINK_ATTR  A_NORMAL | COLOR_PAIR(6)
#define SOCK_ATTR  A_NORMAL | COLOR_PAIR(1)
#define FIFO_ATTR  A_NORMAL | COLOR_PAIR(5)
#define EXEC_ATTR  A_NORMAL | COLOR_PAIR(2)

#define CONTROL(c)  ((c) ^ 0x40)

struct cpair {
	int fg;
	int bg;
};

/* Colors to use with COLOR_PAIR(n) as attributes */
struct cpair pairs[] = {
	{ .fg = 0, .bg = 0 },
	/* pairs start at 1 */
	{ COLOR_RED,     -1 },  /* COLOR_PAIR(1) */
	{ COLOR_GREEN,   -1 },  /* COLOR_PAIR(2) */
	{ COLOR_YELLOW,  -1 },  /* COLOR_PAIR(3) */
	{ COLOR_LIGHTBLUE,-1 }, /* COLOR_PAIR(4) */
	{ COLOR_MAGENTA, -1 },  /* COLOR_PAIR(5) */
	{ COLOR_CYAN,    -1 },  /* COLOR_PAIR(6) */
};

/* Supported actions */
enum action {
	SEL_QUIT = 1,
	SEL_BACK,
	SEL_GOIN,
	SEL_FLTR,
	SEL_NEXT,
	SEL_PREV,
	SEL_PGDN,
	SEL_PGUP,
	SEL_HOME,
	SEL_END,
	SEL_CD,
	SEL_CDHOME,
	SEL_TOGGLEDOT,
	SEL_DSORT,
	SEL_SSIZE,
	SEL_MTIME,
	SEL_ICASE,
	SEL_VERS,
	SEL_REDRAW,
	SEL_HELP,
	SEL_RUN
};

enum runtype {
    Noargs,             /* exec cmd w/no arguments */
    Curname,            /* exec cmd and pass current filename as %s argument */
    Runcurname,         /* exec current filename w/no argument */
    Runshell            /* run sh -c on cmd and %s argument */
};

struct rule {
    char *matchstr;     /* regular matching expression */
    char *cmd;          /* program or command line to run */
    enum runtype type;  /* type of command line */
};

struct rule rules[] = {
    { "*.[123456789].Z",  "man",                                  Curname },
    { "*.[123456789]",    "man",                                  Curname },
    { "*.Z",              "exec compress -dc %s | more",          Runshell },
    { "*",                "more",                                 Curname }
};

struct key {
    int sym;         	    /* Key pressed */
    enum action act; 	    /* Action */
    char *run;       	    /* Program to run or command line */
    enum runtype type;      /* Type of command line */
};

struct key bindings[] = {
	{ 'Z',            SEL_QUIT, 0,0 },

	{ kBackSpace,     SEL_BACK, 0,0 },       /* exit directory */
	{ kDel,           SEL_BACK, 0,0 },
	{ kLeftArrow,     SEL_BACK, 0,0 },
	{ CONTROL('H'),   SEL_BACK, 0,0 },

	{ '\r',           SEL_GOIN, 0,0 },       /* enter directory or display file */
	{ kRightArrow,    SEL_GOIN, 0,0 },
	{ kMouseLeftDoubleClick,SEL_GOIN, 0,0 },

	{ kDownArrow,     SEL_NEXT, 0,0 },       /* next entry */
	{ kMouseWheelDown,SEL_NEXT, 0,0 },
	{ CONTROL('N'),   SEL_NEXT, 0,0 },

	{ kUpArrow,       SEL_PREV, 0,0 },       /* previous entry */
	{ CONTROL('P'),   SEL_PREV, 0,0 },
	{ kMouseWheelUp,  SEL_PREV, 0,0 },

	{ kPageDown,      SEL_PGDN, 0,0 },       /* more next entries */
	{ CONTROL('D'),   SEL_PGDN, 0,0 },

	{ kPageUp,        SEL_PGUP, 0,0 },       /* more previous entries */
	{ CONTROL('U'),   SEL_PGUP, 0,0 },

	{ kHome,          SEL_HOME, 0,0 },       /* first entry */
	{ '^',            SEL_HOME, 0,0 },
	{ 'H',            SEL_HOME, 0,0 },

	{ kEnd,           SEL_END, 0,0 },        /* last entry */
	{ '$',            SEL_END, 0,0 },
	{ 'B',            SEL_END, 0,0 },

	{ '/',            SEL_FLTR, 0,0 },       /* file filter */

	{ 'C',            SEL_CD, 0,0 },         /* Change dir */
	{ '~',            SEL_CDHOME, 0,0 },     /* Change to home dir */

	{ '.',            SEL_TOGGLEDOT, 0,0 },  /* Tottle hide .dot files */
	{ 'D',            SEL_DSORT, 0,0 },      /* Toggle sort by directory first */
	{ 'S',            SEL_SSIZE, 0,0 },      /* Toggle sort by size */
	{ 'T',            SEL_MTIME, 0,0 },      /* Toggle sort by time */
	{ 'I',            SEL_ICASE, 0,0 },      /* Toggle case sensitivity */
	{ 'V',            SEL_VERS, 0,0 },       /* Toggle sort by version number */

	{ CONTROL('L'),   SEL_REDRAW, 0,0 },
	{ '?',            SEL_HELP, 0,0 },

	{ '!',            SEL_RUN, "sh",                            Noargs },
	{ 'E',            SEL_RUN, "vi",                            Curname },
	{ 'R',            SEL_RUN, "./%s",                          Runcurname }
};
