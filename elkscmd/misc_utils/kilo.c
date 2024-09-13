/* Kilo -- A very simple editor in less than 1-kilo lines of code (as counted
 *         by "cloc"). Does not depend on libcurses, directly emits VT100
 *         escapes on the terminal.
 *
 * -----------------------------------------------------------------------
 *
 * Copyright (C) 2016 Salvatore Sanfilippo <antirez at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define KILO_VERSION "0.5"

#define _BSD_SOURCE
#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include <ctype.h> 
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* Syntax highlight types */
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2   /* Single line comment. */
#define HL_MLCOMMENT 3 /* Multi-line comment. */
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8      /* Search match. */

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

#define KILO_QUERY_LEN 80 //256
#define TABSPACE 4

struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[2];
    char multiline_comment_start[3];
    char multiline_comment_end[3];
    int flags;
};

/* This structure represents a single line of the file we are editing. */
typedef struct erow {
    int idx;            /* Row index in the file, zero-based. */
    int size;           /* Size of the row, excluding the null term. */
    int rsize;          /* Size of the rendered row. */
    char *chars;        /* Row content. */
    char *render;       /* Row content "rendered" for screen (for TABs). */
    unsigned char *hl;  /* Syntax highlight type for each character in render.*/
    int hl_oc;          /* Row had open comment at end in last syntax highlight
                           check. */
} erow;

/*
typedef struct hlcolor {
    int r,g,b;
} hlcolor;
*/

struct bm {
    int linenum;
    char reminders[21];
};

struct editorConfig {
    int cx,cy;  /* Cursor x and y position in characters */
    int rowoff;     /* Offset of row displayed. */
    int coloff;     /* Offset of column displayed. */
    int screenrows; /* Number of rows that we can show */
    int screencols; /* Number of cols that we can show */
    int numrows;    /* Number of rows */
    int rawmode;    /* Is terminal raw mode enabled? */
    erow *row;      /* Rows */
    int dirty;      /* File modified but not saved. */
    char *filename; /* Currently open filename */
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;    /* Current syntax highlight, or NULL. */
};
static struct editorConfig E;

struct recentfiles{
     int cx;
     int cy;
     int coloff;
     int rowoff;
     char recentfilename[128];
};

#define max_recent 10
#define kilorecentfile ".kilorecent"
static struct recentfiles rf[max_recent]; 

char* copyText = NULL;
int copyTopRow, copyBottomRow = 0;
int helpFlag = 0;
static struct bm bookmark[8];
int bmFlag = 0;
int rfFlag = 0;

enum KEY_ACTION{            /* unused: D,G,J,K,U, */
        KEY_NULL = 0,       /* NULL */
        CTRL_A = 1,         /* Ctrl-a Help page */
        CTRL_B = 2,         /* Ctrl-b bookmarks */
        CTRL_C = 3,         /* Ctrl-c copy lines */
        //CTRL_D = 4,         /* Ctrl-d */
        CTRL_E = 5,         /* Ctrl-e end of file*/
        CTRL_F = 6,         /* Ctrl-f find */
        CTRL_H = 8,         /* Ctrl-h backspace */
        TAB = 9,            /* Tab */
        CTRL_J = 10,        /* Ctrl-j  list recent files*/
        CTRL_L = 12,        /* Ctrl+l insert file*/
        ENTER = 13,         /* Enter */
        CTRL_N = 14, 	    /* New */
        CTRL_O = 15, 	    /* Open */
        CTRL_P = 16,		/* Ctrl-p */
        CTRL_Q = 17,        /* Ctrl-q exit */
        CTRL_R = 18,        /* Ctrl-r replace */
        CTRL_S = 19,        /* Ctrl-s save file */
        CTRL_T = 20,        /* Ctrl-t top of file*/        
        //CTRL_U = 21,        /* Ctrl-u */
        CTRL_V = 22,	    /* Ctrl-v paste line*/
        CTRL_W = 23,	    /* Ctrl-w saveas */        
        CTRL_X = 24,	    /* CTRL-X delete line*/
        CTRL_Y = 25,	    /* CTRL-Y get memory*/
        CTRL_Z = 26,		/* ctrl-z undo */
        ESC = 27,           /* Escape */
        DELETE =  127,      /* Delete */
        /* The following are just soft codes, not really reported by the
         * terminal directly. */
        ARROW_LEFT = 1000,
        ARROW_RIGHT,
        ARROW_UP,
        ARROW_DOWN,
        DEL_KEY,
        HOME_KEY,
        END_KEY,
        PAGE_UP,
        PAGE_DOWN,
        INS_KEY
};

void editorSetStatusMessage(const char *fmt, ...);

void editorMoveCursor(int key);
void editorRefreshScreen(); // forward declare to avoid compile exception
void copylines(int numlines);
void pastebuffer(void);
void editorSaveAs(void);
void initEditor(void);
void editorDelChar();
int bufferToFile(void);
void copybuffer(void);
void clearScreen(void);
void showRecentCurPos(int rf_number, char* label);

/* =========================== Syntax highlights DB =========================
 *
 * In order to add a new syntax, define two arrays with a list of file name
 * matches and keywords. The file name matches are used in order to match
 * a given syntax with a given file name: if a match pattern starts with a
 * dot, it is matched as the last past of the filename, for example ".c".
 * Otherwise the pattern is just searched inside the filenme, like "Makefile").
 *
 * The list of keywords to highlight is just a list of words, however if they
 * a trailing '|' character is added at the end, they are highlighted in
 * a different color, so that you can have two different sets of keywords.
 *
 * Finally add a stanza in the HLDB global variable with two two arrays
 * of strings, and a set of flags in order to enable highlighting of
 * comments and numbers.
 *
 * The characters for single and multi line comments must be exactly two
 * and must be provided as well (see the C language example).
 *
 * There is no support to highlight patterns currently. */

/* C / C++ */
char *C_HL_extensions[] = {".c",".cpp",".cc",".h",".hpp",NULL};
char *C_HL_keywords[] = {
    /* C Keywords */
	"break","case","continue","default","do","else","enum",
	"extern","for","goto","if","register","return","sizeof","static",
	"struct","switch","typedef","union","volatile","while","NULL",
	/* C++ Keywords */
	"alignas","alignof","and","and_eq","asm","bitand","bitor","class",
	"compl","constexpr","const_cast","deltype","delete","dynamic_cast",
	"explicit","export","false","friend","inline","mutable","namespace",
	"new","noexcept","not","not_eq","nullptr","operator","or","or_eq",
	"private","protected","public","reinterpret_cast","static_assert",
	"static_cast","template","this","thread_local","throw","true","try",
	"typeid","typename","virtual","xor","xor_eq",
     /* C types */
     "int|","long|","double|","float|","char|","unsigned|","signed|",
     "void|","short|","auto|","const|",NULL
};

// Python
char *PY_HL_extensions[] = { ".py", NULL };
char *PY_HL_keywords[] = {
    // Python keywords and built-in functions
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else", "except", "exec", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try", "while", "with", "yield", "async", "await",
    "nonlocal", "range", "xrange", "reduce", "map", "filter", "all", "any", "sum", "dir", "abs", "breakpoint", "compile", "delattr", "divmod",
    "format", "eval", "getattr", "hasattr", "hash", "help", "id", "input", "isinstance", "issubclass", "len", "locals", "max", "min", "next",
    "open", "pow", "repr", "reversed", "round", "setattr", "slice", "sorted", "super", "vars", "zip", "__import__", "reload", "raw_input",
    "execfile", "file", "cmp", "basestring",
    // Python types
    "buffer|", "bytearray|", "bytes|", "complex|", "float|", "frozenset|", "int|", "list|", "long|", "None|", "set|", "str|", "chr|", "tuple|",
    "bool|", "False|", "True|", "type|", "unicode|", "dict|", "ascii|", "bin|", "callable|", "classmethod|", "enumerate|", "hex|", "oct|", "ord|",
    "iter|", "memoryview|", "object|", "property|", "staticmethod|", "unichr|", NULL
};

// Go
char *GO_HL_extensions[] = { ".go", NULL };
char *GO_HL_keywords[] = {
    // Go keywords
    "if", "for", "range", "while", "defer", "switch", "case", "else", "func", "package", "import", "type", "struct", "import", "const", "var",
    // Go types
    "nil|", "true|", "false|", "error|", "err|", "int|", "int32|", "int64|", "uint|", "uint32|", "uint64|", "string|", "bool|", NULL
};

// JavaScript
char *JS_HL_extensions[] = { ".js", ".jsx", NULL };
char *JS_HL_keywords[] = {
    // JavaScript keywords
    "break", "case", "catch", "class", "const", "continue", "debugger", "default", "delete", "do", "else", "enum", "export", "extends", "finally", "for", "function", "if", "implements", "import", "in", "instanceof", "interface", "let", "new", "package", "private", "protected", "public", "return", "static", "super", "switch", "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield", "true", "false", "null", "NaN", "global", "window", "prototype", "constructor", "document", "isNaN", "arguments", "undefined",
    // JavaScript primitives
    "Infinity|", "Array|", "Object|", "Number|", "String|", "Boolean|", "Function|", "ArrayBuffer|", "DataView|", "Float32Array|", "Float64Array|", "Int8Array|", "Int16Array|", "Int32Array|", "Uint8Array|", "Uint8ClampedArray|", "Uint32Array|", "Date|", "Error|", "Map|", "RegExp|", "Symbol|", "WeakMap|", "WeakSet|", "Set|", NULL
};

/* Here we define an array of syntax highlights by extensions, keywords,
 * comments delimiters and flags. */
struct editorSyntax HLDB[] = {
    {
        /* C / C++ */
        C_HL_extensions,
        C_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        PY_HL_extensions,
        PY_HL_keywords,
        "#","\"\"\"", "\"\"\"",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    // python multi_line_comment_start and _end = """ , triple-quoted strings
    {
        GO_HL_extensions,
        GO_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        JS_HL_extensions,
        JS_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
    
};

#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))

/* ======== History for undo - builds and maintains link list of events ======== */
/*
When you enter a space character, a Tab or press the Enter key, an Undo block begins.
If you press Ctrl-z then, Kilo will erase all that you entered up to the last beginning
of an Undo block. The last cursor position before the beginning of a block is saved and
an undo command will return to that position. It will not erase a newline done after an
Enter key, it will just erase entered characters. It will not erase the first character
of a file.
Does free() the node while executing an undo command.
*/

// should replace turnOffTracing() with recordkeystrokes - keep while testing

// stack
#define TRUE 1
#define FALSE 0

typedef struct _History { // singleton object
	int len;
	int cx;
	int cy;
	short isTracing;
}History;

typedef struct _node {
	History data;
	struct _node* next;
}Node;

typedef struct _listStack {
	Node* head;
}ListStack;

typedef ListStack Stack;
Stack stack; // global-variable
History ht;	  // global-variable

void StackInit() {
	stack.head = NULL;
}
void HistoryInit() {
	ht.isTracing = TRUE;
	ht.len = 0;
	ht.cx = 0;
	ht.cy = 0;
}
int SIsEmpty() {
	if(stack.head == NULL)
		return TRUE;
	else 
		return FALSE;
}
void SPush() {
	Node* newNode = (Node*)malloc(sizeof(Node));
	History temp = ht;
	newNode->data = temp;
	newNode->next = stack.head;
	stack.head = newNode;
}

void turnOffTracing() {
	if(ht.isTracing) {
		SPush();
		HistoryInit();
	}
}

History SPop() {
	History rdata;
	Node* rnode;

	if(SIsEmpty(stack)) {
		printf("Stack Memory Error!");
		exit(-1);
	}

	rdata = stack.head->data;
	rnode = stack.head;

	stack.head = stack.head->next;
	free(rnode);

	return rdata;
}
History SPeek() {
	if(SIsEmpty(stack)) {
		printf("Stack Memory Error!");
		exit(-1);
	}

	return stack.head->data;
}

void undo() {
	History temp;

	if(ht.len) 
		turnOffTracing();

	if(SIsEmpty())
		return;

	temp = SPop();

    E.cx = temp.cx + 1;
	E.cy = temp.cy;
	editorRefreshScreen();
    
	for(int i=0; i<temp.len; i++)  
		editorDelChar();
/*    
	if(temp.len > 1)
		editorDelChar();
	printf("%d ", E.cx);
	while(!SIsEmpty()) {
		printf("%d ", SPop().cx);
	}
	exit(1);
*/	
	HistoryInit();
}

/* ======== List of recently opened files ======== */

void storeRecentCurPos() {
    rf[0].cx=E.cx; rf[0].cy=E.cy;
    rf[0].coloff=E.coloff; rf[0].rowoff=E.rowoff;
}

void loadRecentCurPos(int rf_number) {
    E.cx=rf[rf_number].cx; E.cy=rf[rf_number].cy;
    E.coloff=rf[rf_number].coloff; E.rowoff=rf[rf_number].rowoff;
}

void showRecentCurPos(int rf_number, char* label) {
#if 0
    printf("\r\n\n\n"); //clearScreen();
    printf("%s, %s,%d,%d,%d,%d\r\n",label, rf[rf_number].recentfilename,rf[rf_number].cx, 
           rf[rf_number].cy, rf[rf_number].coloff, rf[rf_number].rowoff);
    printf("%s, %d,%d,%d,%d\r\n",E.filename, E.cx, E.cy, E.coloff, E.rowoff);
    fflush(stdout);
    sleep(3);
    editorRefreshScreen();
#endif    
}

void add_to_recentfiles() {
    /* store current file in first entry of list of recent files - called when loading the file in editorOpen*/
    if (rf[0].recentfilename[0] != '\0') { /*push entries down, erase the last */
        storeRecentCurPos();
        for(int i=(max_recent-1); i>0; i--)
            { rf[i] = rf[i-1]; }
    }
    snprintf(rf[0].recentfilename,200,E.filename);
    storeRecentCurPos();
}

int saverecentfiles(){
    FILE * fptr;
    int i;
    char filenamestr[] = kilorecentfile;
    storeRecentCurPos();
    unlink(filenamestr);
    fptr = fopen (filenamestr,"w");
    if (fptr == NULL) return -1;

	for(i=0; i<max_recent; i++) {
        fprintf(fptr, "%s %d %d %d %d ", rf[i].recentfilename, rf[i].cx, rf[i].cy, rf[i].coloff, rf[i].rowoff);
	}
    fclose(fptr);
    return 0;    
}

int readrecentfiles(){
    FILE * fptr;
    int i;
    char filenamestr[] = kilorecentfile;
    
    fptr = fopen (filenamestr,"r");
    if (fptr == NULL) {
        /* no file found, init */
        rf[0].recentfilename[0] = '\0';
        return -1;
    }

	for(i=0; i<max_recent; i++) {
        fscanf(fptr, "%s %d %d %d %d", rf[i].recentfilename, &rf[i].cx, &rf[i].cy, &rf[i].coloff, &rf[i].rowoff);
	}
    fclose(fptr);
    
    return 0;
}

/* ======================= Low level terminal handling ====================== */

static struct termios orig_termios; /* In order to restore at exit.*/

void disableRawMode(int fd) {
    /* Don't even check the return value as it's too late. */
    if (E.rawmode) {
        tcsetattr(fd,TCSAFLUSH,&orig_termios);
        E.rawmode = 0;
    }
}

/* Called at exit to avoid remaining in raw mode. */
void editorAtExit(void) {
    disableRawMode(STDIN_FILENO);
}

/* Raw mode: 1960 magic shit. */
int enableRawMode(int fd) {
    struct termios raw;

    if (E.rawmode) return 0; /* Already enabled. */
    if (!isatty(STDIN_FILENO)) goto fatal;
    atexit(editorAtExit);
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0; /* Timed wait for input. */
    raw.c_cc[VTIME] = 2; /* 200 ms timeout (unit is tens of second). */
    //raw.c_cc[VMIN] = 1; /* Wait for each byte. */
    //raw.c_cc[VTIME] = 0; /* No timeout. */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    E.rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
int editorReadKey(int fd) {
    int nread;
    unsigned char c, seq[3];
    while ((nread = read(fd,&c,1)) == 0);
    if (nread == -1) exit(1);

    while(1) {
        switch((unsigned int)c) {
        case ESC:    /* escape sequence */
            /* If this is just an ESC, we'll timeout here. */
            if (read(fd,seq,1) == 0) return ESC;
            if (read(fd,seq+1,1) == 0) return ESC;

            /* ESC [ sequences. */
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    /* Extended escape, read additional byte. */
                    if (read(fd,seq+2,1) == 0) return ESC;
                    if (seq[2] == '~') {
                        switch(seq[1]) {
                        case '2': return INS_KEY;
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        }
                    }
                } else {
                    switch(seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                    }
                }
            }

            /* ESC O sequences. */
            else if (seq[0] == 'O') {
                switch(seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                }
            }
            break;
        default:
            return c;
        }
    }
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int getCursorPosition(int ifd, int ofd, int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",rows,cols) != 2) return -1;
    return 0;
}

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error. */
int getWindowSize(int ifd, int ofd, int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int orig_row, orig_col, retval;

        /* Get the initial position so we can restore it later. */
        retval = getCursorPosition(ifd,ofd,&orig_row,&orig_col);
        if (retval == -1) goto failed;

        /* Go to right/bottom margin and get position. */
        if (write(ofd,"\x1b[999C\x1b[999B",12) != 12) goto failed;
        retval = getCursorPosition(ifd,ofd,rows,cols);
        if (retval == -1) goto failed;

        /* Restore position. */
        char seq[32];
        snprintf(seq,32,"\x1b[%d;%dH",orig_row,orig_col);
        if (write(ofd,seq,strlen(seq)) == -1) {
            /* Can't recover... */
        }
        return 0;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

failed:
    return -1;
}

void editorQueryString(char* prompt, char *buffer){
    int c, len=0;
	while(1) {
		editorSetStatusMessage("%s %s ", prompt, buffer);
		editorRefreshScreen();
		c = editorReadKey(0);

		if (c == CTRL_H) { /* backspace */
			if(len != 0)
				buffer[--len] = '\0';
		} else if(c == ESC) {
			editorSetStatusMessage("");
            buffer[0] = '\0';
			return;
		} else if(c == ENTER) {
            editorSetStatusMessage("");
			return;
		} else if(isprint(c & 255)) {
			buffer[len++] = c;
			buffer[len] = '\0';
		}
	}
}

void clearScreen() //function to clear the screen
{
  const char *CLEAR_SCREEN_ANSI = "\033[1;1H\033[2J";
  write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 10);
}

#if 0
int checkAvailableMemory()
{
    int Kb = 0;
    int i = 0;
    void *r[64];

    for (i=0; i<64; i++) {
        r[i]=malloc(1024);
        if (r[i] != NULL) {
            Kb++;
        } else {
            break;
        }
    }
    for (int j=0;j<i;j++) free(r[j]);
    return Kb;
}
#endif

void editorBookmarks(int fd, int bm_number)
{
    int c;
    
    if (bm_number == 52) { /* 'd' to display */
        bmFlag = 1;
        editorSetStatusMessage("Enter the number of the selected bookmark:");
        editorRefreshScreen();
        c = editorReadKey(fd);
        if(c == ESC || c == CTRL_B) {
            editorSetStatusMessage("");
            bmFlag=0;
            return;
        }        
        bm_number=c-48;
        bmFlag = 0;
        /* fall through */
    }
        
    if (bm_number<1 || bm_number>7) {
        editorSetStatusMessage("Invalid bookmark number");
        editorRefreshScreen();
        return;
    }
        
    if (bookmark[bm_number].linenum ==0) { /* set */
       bookmark[bm_number].linenum = E.rowoff+E.cy +1; 
       snprintf(bookmark[bm_number].reminders,20,"%s",E.row[E.rowoff+E.cy].render);
       editorSetStatusMessage("Bookmark %d set. Line: %d, %s",bm_number,bookmark[bm_number].linenum,bookmark[bm_number].reminders);     
       editorRefreshScreen();
       return;
    }
    
    if (E.rowoff+E.cy +1 == bookmark[bm_number].linenum) { /*delete*/
        bookmark[bm_number].linenum=0;
        bookmark[bm_number].reminders[0]='\0';
        editorSetStatusMessage("Bookmark %d deleted",bm_number);
        editorRefreshScreen();
        return;
    }
    /* select */
    E.cx = 0;
    E.coloff = 0;
	E.cy = 0; 
    E.rowoff = bookmark[bm_number].linenum-1;
    editorSetStatusMessage("");
    editorRefreshScreen();
    
    return;
}
/* ====================== Syntax highlight color scheme  ==================== */

int is_separator(int c) {
    return c == '\0' || isspace(c) || strchr("{},.()+-/*=~%[];<>|&",c) != NULL;
}

/* Return true if the specified row last char is part of a multi line comment
 * that starts at this row or at one before, and does not end at the end
 * of the row but spawns to the next row. */
int editorRowHasOpenComment(erow *row) {
    if (row->hl && row->rsize && row->hl[row->rsize-1] == HL_MLCOMMENT &&
        (row->rsize < 2 || (row->render[row->rsize-2] != '*' ||
                            row->render[row->rsize-1] != '/'))) return 1;
    return 0;
}

/* Set every byte of row->hl (that corresponds to every character in the line)
 * to the right syntax highlight type (HL_* defines). */
int editorUpdateSyntax(erow *row) {
    if (row->size > 0 ) {
        if ((row->hl = (unsigned char*) realloc(row->hl,row->rsize)) == NULL) return -1;
    } else {
        row->hl = 0;
    }
    memset(row->hl,HL_NORMAL,row->rsize);

    if (E.syntax == NULL) return HL_NORMAL; /* No syntax, everything is HL_NORMAL. */

    int i, prev_sep, in_string, in_comment;
    char *p;
    char **keywords = E.syntax->keywords;
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    /* Point to the first non-space char. */
    p = row->render;
    i = 0; /* Current char offset */
    while(*p && isspace(*p)) {
        p++;
        i++;
    }
    prev_sep = 1; /* Tell the parser if 'i' points to start of word. */
    in_string = 0; /* Are we inside "" or '' ? */
    in_comment = 0; /* Are we inside multi-line comment? */

    /* If the previous line has an open comment, this line starts
     * with an open comment state. */
    if (row->idx > 0 && editorRowHasOpenComment(&E.row[row->idx-1]))
        in_comment = 1;

    while(*p) {
        /* Handle // comments. */
        if (!in_string) {
          if (prev_sep && *p == scs[0] && *(p+1) == scs[1]) {
            /* From here to end is a comment */
            memset(row->hl+i,HL_COMMENT,row->rsize-i);
            return 0;
          }
        }

        /* Handle multi line comments. */
        if (in_comment) {
            row->hl[i] = HL_MLCOMMENT;
            if (*p == mce[0] && *(p+1) == mce[1]) {
                row->hl[i+1] = HL_MLCOMMENT;
                p += 2; i += 2;
                in_comment = 0;
                prev_sep = 1;
                continue;
            } else {
                prev_sep = 0;
                p++; i++;
                continue;
            }
        } else if (*p == mcs[0] && *(p+1) == mcs[1]) {
            row->hl[i] = HL_MLCOMMENT;
            row->hl[i+1] = HL_MLCOMMENT;
            p += 2; i += 2;
            in_comment = 1;
            prev_sep = 0;
            continue;
        }

        /* Handle "" and '' */
        if (in_string) {
            row->hl[i] = HL_STRING;
            if (*p == '\\') {
                row->hl[i+1] = HL_STRING;
                p += 2; i += 2;
                prev_sep = 0;
                continue;
            }
            if (*p == in_string) in_string = 0;
            p++; i++;
            continue;
        } else {
            if (*p == '"' || *p == '\'') {
                in_string = *p;
                row->hl[i] = HL_STRING;
                p++; i++;
                prev_sep = 0;
                continue;
            }
        }

        /* Handle non printable chars. */
        if (!isprint(*p & 255)) {
            row->hl[i] = HL_NONPRINT;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        /* Handle numbers */
        if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) ||
            (*p == '.' && i >0 && row->hl[i-1] == HL_NUMBER)) {
            row->hl[i] = HL_NUMBER;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        if (is_separator(*p)) {
            row->hl[i] = HL_KEYWORD1;
        }
        
        /* Handle keywords and lib calls */
        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen-1] == '|';
                if (kw2) klen--;

                if (strlen(p) >=klen && !memcmp(p,keywords[j],klen) &&
                    is_separator(*(p+klen)))
                {
                    /* Keyword */
                    memset(row->hl+i,kw2 ? HL_KEYWORD2 : HL_KEYWORD1,klen);
                    p += klen;
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue; /* We had a keyword match */
            }
        }

        /* Not special chars */
        prev_sep = is_separator(*p);
        p++; i++;
    }

    /* Propagate syntax change to the next row if the open comment
     * state changed. This may recursively affect all the following rows
     * in the file. */
    int oc = editorRowHasOpenComment(row);
    if (row->hl_oc != oc && row->idx+1 < E.numrows)
        if (editorUpdateSyntax(&E.row[row->idx+1]) == -1) return -1;
    row->hl_oc = oc;
    
    return 0;
}

/* ANSI colors:
 * 31=red, 32=green, 33=yellow, 34=blue, 35=magenta, 36=cyan, 37=white
 */
/* Maps syntax highlight token types to terminal colors. */
int editorSyntaxToColor(int hl) {
    switch(hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 36;     /* cyan */
    case HL_KEYWORD1: return 33;    /* yellow */
    case HL_KEYWORD2: return 32;    /* green */
    case HL_STRING: return 35;      /* magenta */
    case HL_NUMBER: return 31;      /* red */
    case HL_MATCH: return 7; /*invert*/  //34; /* blu */
    default: return 37;             /* white */
    }
}

/* Select the syntax highlight scheme depending on the filename,
 * setting it in the global state E.syntax. */
void editorSelectSyntaxHighlight(char *filename) {
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = HLDB+j;
        unsigned int i = 0;
        while(s->filematch[i]) {
            char *p;
            int patlen = strlen(s->filematch[i]);
            if ((p = strstr(filename,s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    E.syntax = s;
                    return;
                }
            }
            i++;
        }
    }
}

/* ======================= Editor rows implementation ======================= */

/* Update the rendered version and the syntax highlight of a row. */
int editorUpdateRow(erow *row) {
    int tabs = 0, j, idx;

   /* Create a version of the row we can directly print on the screen,
     * respecting tabs, substituting non printable characters with '?'. */
    free(row->render);
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == TAB) tabs++;

    if ((row->render = malloc(row->size + tabs*TABSPACE + 1)) == NULL) {
        return -1;
    }
    idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == TAB) {
            row->render[idx++] = ' ';
            while((idx+1) % TABSPACE != 0) row->render[idx++] = ' ';
        } else if (!isprint(row->chars[j] & 255)) {
            row->render[idx++] = '?';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->rsize = idx;
    row->render[idx] = '\0';

    /* Update the syntax highlighting attributes of the row. */
    if (editorUpdateSyntax(row) == -1) return -1;
    return 0;
}

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
int editorInsertRow(int at, char *s, size_t len) {
    if (at > E.numrows) return -1;
    if ((E.row = realloc(E.row,sizeof(erow)*(E.numrows+1))) == NULL) { 
        return -1; 
    }
    if (at != E.numrows) {
        memmove(E.row+at+1,E.row+at,sizeof(E.row[0])*(E.numrows-at));
        for (int j = at+1; j <= E.numrows; j++) E.row[j].idx++;
    }
    E.row[at].size = len;
    if ((E.row[at].chars = malloc(len+1))==0) {
        return -1; 
    }
    memcpy(E.row[at].chars,s,len+1);
    E.row[at].hl = NULL;
    E.row[at].hl_oc = 0;
    E.row[at].render = NULL;
    E.row[at].rsize = 0;
    E.row[at].idx = at;
    if (editorUpdateRow(E.row+at) == -1) { 
        return -1;
    }
    E.numrows++;
    E.dirty++;
    return 0;
}

/* Free row's heap allocated stuff. */
void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

/* Remove the row at the specified position, shifting the remainign on the
 * top. */
void editorDelRow(int at) {
    erow *row;

    if (at >= E.numrows) return;
    row = E.row+at;
    editorFreeRow(row);
    memmove(E.row+at,E.row+at+1,sizeof(E.row[0])*(E.numrows-at-1));
    for (int j = at; j < E.numrows-1; j++) E.row[j].idx--;
    E.numrows--;
    E.dirty++;
}

#if 0
/* Turn the editor rows into a single heap-allocated string.
 * Returns the pointer to the heap-allocated string and populate the
 * integer pointed by 'buflen' with the size of the string, escluding
 * the final nulterm. */
char *editorRowsToString(int *buflen) {
    char *buf = NULL, *p;
    int totlen = 0;
    int j;

    /* Compute count of bytes */
    for (j = 0; j < E.numrows; j++)
        totlen += E.row[j].size+1; /* +1 is for "\n" at end of every row */
    *buflen = totlen;
    totlen++; /* Also make space for nulterm */

    p = buf = malloc(totlen);
    for (j = 0; j < E.numrows; j++) {
        memcpy(p,E.row[j].chars,E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    *p = '\0';
    return buf;
}
#endif

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void editorRowInsertChar(erow *row, int at, int c) {
    if (at > row->size) {
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character. */
        int padlen = at-row->size;
        /* In the next line +2 means: new char and null term. */
        row->chars = realloc(row->chars,row->size+padlen+2);
        memset(row->chars+row->size,' ',padlen);
        row->chars[row->size+padlen+1] = '\0';
        row->size += padlen+1;
    } else {
        /* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term. */
        row->chars = realloc(row->chars,row->size+2);
        memmove(row->chars+at+1,row->chars+at,row->size-at+1);
        row->size++;
    }
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

/* Append the string 's' at the end of a row */
void editorRowAppendString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars,row->size+len+1);
    memcpy(row->chars+row->size,s,len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

/* Delete the character at offset 'at' from the specified row. */
void editorRowDelChar(erow *row, int at) {
    if (row->size <= at) return;
    memmove(row->chars+at,row->chars+at+1,row->size-at);
    editorUpdateRow(row);
    row->size--;
    E.dirty++;
}

/* Insert the specified char at the current prompt position. */
void editorInsertChar(int c) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    
    /* add lines for undo feature */
    if(c == TAB) {
		ht.cx = filecol;
		ht.cy = filerow;        
		turnOffTracing();
		ht.len = 1;        
		ht.cx = filecol;
		ht.cy = filerow;
	} else if(isspace(c)) {
		ht.cx = filecol;
		ht.cy = filerow;
        turnOffTracing();
		ht.len = 1;
		ht.cx = filecol;
		ht.cy = filerow;
	} else if(ht.isTracing == TRUE) {
		ht.cx = filecol;
		ht.cy = filerow;
		ht.len++;
	}
    
    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */
    if (!row) {
        while(E.numrows <= filerow)
            editorInsertRow(E.numrows,"",0);
    }
    row = &E.row[filerow];
    editorRowInsertChar(row,filecol,c);
    if (E.cx == E.screencols-1)
        E.coloff++;
    else
        E.cx++;
    E.dirty++;
}

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed. */
void editorInsertNewline(void) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row) {
        if (filerow == E.numrows) {
            editorInsertRow(filerow,"",0);
            goto fixcursor;
        }
        return;
    }
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character. */
    if (filecol >= row->size) filecol = row->size;
    if (filecol == 0) {
        editorInsertRow(filerow,"",0);
    } else {
        /* We are in the middle of a line. Split it between two rows. */
        editorInsertRow(filerow+1,row->chars+filecol,row->size-filecol);
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        editorUpdateRow(row);
    }
fixcursor:
    if (E.cy == E.screenrows-1) {
        E.rowoff++;
    } else {
        E.cy++;
    }
    E.cx = 0;
    E.coloff = 0;
}

/* Delete the char at the current prompt position. */
void editorDelChar() {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    if (!row || (filecol == 0 && filerow == 0)) return;
    if (filecol == 0) {
        /* Handle the case of column 0, we need to move the current line
         * on the right of the previous one. */
        filecol = E.row[filerow-1].size;
        editorRowAppendString(&E.row[filerow-1],row->chars,row->size);
        editorDelRow(filerow);
        row = NULL;
        if (E.cy == 0)
            E.rowoff--;
        else
            E.cy--;
        E.cx = filecol;
        if (E.cx >= E.screencols) {
            int shift = (E.screencols-E.cx)+1;
            E.cx -= shift;
            E.coloff += shift;
        }
    } else {
        editorRowDelChar(row,filecol-1);
        if (E.cx == 0 && E.coloff)
            E.coloff--;
        else
            E.cx--;
    }
    if (row) editorUpdateRow(row);
    E.dirty++;
}

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editorOpen(char *filename) {
    FILE *fp;
    int partial;

    E.dirty = 0;
    free(E.filename);
    E.filename = strdup(filename);

    fp = fopen(filename,"r");
    if (!fp) {
        if (errno != ENOENT) {
            perror("Opening file");
            exit(1);
        }
        return 1;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    partial=0;
    while((linelen = getline(&line,&linecap,fp)) != -1) {
        if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
            line[--linelen] = '\0';
        partial = editorInsertRow(E.numrows,line,linelen);
        if (partial == -1) break;
    }
    add_to_recentfiles(); //rf[0].recentfilename[0] should be set to zero before calling editorOpen
    free(line);
    fclose(fp);
    E.dirty = 0;
    if (partial == -1) return -1;
    return 0;
}

/* Insert the specified file in the editor memory and returns 0 on success
 * or 1 on error. */
int editorInsertFile(int at) {
    FILE *fp;
    int partial;
    
    char namebuffer[300] = {0,};
    /* Save the cursor position in order to restore it later. */
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;
    
    editorQueryString("Please enter filename:",namebuffer); 
    if (namebuffer[0]=='\0') return -1;
    
    fp = fopen(namebuffer,"r");
    if (!fp) {
        if (errno != ENOENT) {
            perror("Opening file");
            exit(1);
        }
        return -1;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    int i=0;
    partial=0;

    while((linelen = getline(&line,&linecap,fp)) != -1) {
        if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
            line[--linelen] = '\0';
        partial = editorInsertRow(at+i,line,linelen);
        if (partial == -1) break;
        i++;
    }
    free(line);
    fclose(fp);
    
    E.cx = saved_cx; E.cy = saved_cy;
    E.coloff = saved_coloff; E.rowoff = saved_rowoff;

    editorSetStatusMessage(
        "Ctrl-A = help | Ctrl-Q = quit");
    editorRefreshScreen();
    
    if (partial == -1) return -1;
    return 0;
}

/* Save the current file on disk. Return 0 on success, 1 on error. */
int editorSave(void) {
    int len, fd, j;
    char nl[2] = "\n";
    /* Compute count of bytes */
    len=0;
    for (j = 0; j < E.numrows; j++)
        len += E.row[j].size+1; /* +1 is for "\n" at end of every row */

    char comparestr[] = "unknown.txt";

    unlink(E.filename);
    if(strcmp(E.filename,comparestr) == 0) {
        editorSaveAs();
        return 0;        
    }
    fd = open(E.filename,O_RDWR|O_CREAT,0644);
    if (fd == -1) goto writeerr;

    for (j = 0; j < E.numrows; j++) {
        if (write(fd,strncat(E.row[j].chars,nl,1),E.row[j].size+1) != E.row[j].size+1) goto writeerr;
    }

    close(fd);
    E.dirty = 0;
    editorSetStatusMessage("%d bytes written on disk", len);
    return 0;

writeerr:
    if (fd != -1) close(fd);
    editorSetStatusMessage("Can't save! I/O error: %s",strerror(errno));
    return 1;
}

void editorSaveAs(void){
    char namebuffer[300] = {0,};
    editorQueryString("Please enter filename:",namebuffer); 
    if (namebuffer[0]=='\0') return;
    sprintf(E.filename,"%s",namebuffer);
    snprintf(rf[0].recentfilename,200,E.filename);
    storeRecentCurPos();
    editorSave();

    return;
}

int editorOpenNew(void){
    char namebuffer[300] = {0,};
    int result, i;
    erow *row;
    
    editorQueryString("Please enter filename:",namebuffer); 
    if (namebuffer[0]=='\0') return -1;

    for (i=0;i<E.numrows;i++) {
            row = E.row+i;
            editorFreeRow(row);
    }
    initEditor();
    editorSelectSyntaxHighlight(namebuffer);
    result = editorOpen(namebuffer);

    editorSetStatusMessage(
        "Ctrl-A = help | Ctrl-Q = quit");
    editorRefreshScreen();
    return result;
}


/* ============================= Terminal update ============================ */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL,0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

void removeOneLine() {
    int linesize;    
    
    E.cx = E.row[E.rowoff+E.cy].size;
    linesize=E.cx;
	for(int i=0; i<linesize+1; i++) 		
		editorDelChar();
    
    /* position cursor at first column in row below */   
    editorMoveCursor(ARROW_DOWN);
    E.cx = 0; /* Home */
    /* workaround to update syntax highlighting */
    editorDelChar();
    editorInsertNewline();
}

/* This function writes the whole screen using VT100 escape characters
 * starting from the logical state of the editor in the global state 'E'. */
void editorRefreshScreen(void) {
    int y;
    erow *r;
    char buf[32];
	//char background_color[32];
    struct abuf ab = ABUF_INIT;

    abAppend(&ab,"\x1b[?25l",6); /* Hide cursor. */
    abAppend(&ab,"\x1b[H",3); /* Go home. */
	//abAppend(&ab, background_color, strlen(background_color));
	for (y = 0; y < E.screenrows; y++) {
        int filerow = E.rowoff+y;

        if (filerow >= E.numrows || helpFlag || bmFlag || rfFlag) {
            if ((E.numrows == 0 ||helpFlag|| bmFlag || rfFlag )&& y == E.screenrows/2) {
                char welcome[80];
                int welcomelen = snprintf(welcome,sizeof(welcome),
                    "\x1b[34mKilo editor -- version %s  \x1b[27m\x1b[0K\r\n\x1b[0K~\r\n", KILO_VERSION);
                int padding = (E.screencols-welcomelen)/2;
                if (padding) {
                    abAppend(&ab,"~",1);
                    padding--;
                }
                while(padding--) abAppend(&ab," ",1);

                abAppend(&ab,welcome,welcomelen);

                if (helpFlag) {
                char function[25][90]={"\x1b[K\r",
                "\x1b[34mFile\x1b[K\r\n",
                "\x1b[32mCTRL-N\x1b[36m: Open new empty file\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-O\x1b[36m: Open new file from disk\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-J\x1b[36m: Select file from a list of recently opened files\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-S\x1b[36m: Save edited file\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-W\x1b[36m: Save file as ...\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-A\x1b[36m: Toggle help and re-display current text\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-Q\x1b[36m: Quit the editor\x1b[27m\x1b[K\r\n", 
                "\x1b[34mEdit\x1b[K\r\n",
                //"\x1b[32mInsert\x1b[36m: Toggle insert and overwrite mode\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-C\x1b[36m: Select and copy a block of characters\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-V\x1b[36m: Paste copied block of characters\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-P\x1b[36m: Write the copied block into the file kilobuffer.txt\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-L\x1b[36m: Insert a file from disk at the current position\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-Z\x1b[36m: Undo, remove keys entered after last space, tab or enter key\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-X\x1b[36m: Remove current line and copy it into the buffer\x1b[27m\x1b[K\r\n",
                "\x1b[34mSearch\x1b[K\r\n",
                "\x1b[32mCTRL-F\x1b[36m: Find string in file\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-R\x1b[36m: Find string and replace\x1b[27m\x1b[K\r\n",
                "\x1b[34mPosition cursor\x1b[K\r\n",
                "\x1b[36mHome/End key - move cursor to start/end of line\x1b[27m\x1b[K\r\n",                
                "\x1b[32mCTRL-T\x1b[36m: Move to Top of file\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-E\x1b[36m: Move to End of file\x1b[27m\x1b[K\r\n",
                "\x1b[32mCTRL-B\x1b[36m: Set and select bookmarks\x1b[27m\x1b[K\r\n"};

			    for(int i=0;i<24;++i){
			    	int padding = (E.screencols-welcomelen)/3;
				    while(padding--) abAppend(&ab," ",1);
				    abAppend(&ab,function[i],strlen(function[i]));                    
				}
				abAppend(&ab,"\x1b[0m",4);
                y=y+15; /* adjust for length of list of functions */
                }
                if (bmFlag) {
                    int padding = (E.screencols-welcomelen)/2;
				    while(padding--) abAppend(&ab," ",1);
                    for(int i=0;i<7;++i){
                    char function[10]={"\x1b[K\x1b[32m\r"};
                    
                    abAppend(&ab,function,strlen(function));
                    abAppend(&ab,"              ",14);
                    /* reuse welcome,welcomelen here */
                    welcomelen= snprintf(welcome,80,"Bookmark %d = Line:%4d,   \x1b[36m%s\r\n",i+1,bookmark[i+1].linenum,bookmark[i+1].reminders);
                    abAppend(&ab,welcome,welcomelen);
                    }
                    abAppend(&ab,"\x1b[27m\x1b[K\x1b[0m\r\n",14);
                    y=y+5; /* adjust for length of list of functions */
                }
                if (rfFlag) {
                    int padding = (E.screencols-welcomelen)/2;
				    while(padding--) abAppend(&ab," ",1);
                    char function2[40]={"\x1b[37mRecently loaded files\x1b[K\x1b[32m\r\n"};
                    abAppend(&ab,function2,strlen(function2));
                    abAppend(&ab,"              ",14);
                    for(int i=0;i<(max_recent);++i){
                    char function[10]={"\x1b[K\x1b[32m\r"};                    
                    abAppend(&ab,function,strlen(function));
                    abAppend(&ab,"              ",14);
                    /* reuse welcome,welcomelen here */
                    welcomelen= snprintf(welcome,80,"%d = \x1b[36m (%d-%d) %s\r\n",i+1,rf[i].cx+rf[i].coloff, rf[i].cy+rf[i].rowoff, rf[i].recentfilename);
                    //welcomelen= snprintf(welcome,80,"%d = \x1b[36m%s,%d,%d,%d,%d\r\n",i+1,rf[i].recentfilename,rf[i].cx, 
                    //    rf[i].cy, rf[i].coloff, rf[i].rowoff);                    
                    abAppend(&ab,welcome,welcomelen);
                    }
                    abAppend(&ab,"\x1b[27m\x1b[K\x1b[0m\r\n",14);
                    y=y+5; /* adjust for length of list of functions */
                }
            } else {
                abAppend(&ab,"~\x1b[0K\r\n",7);
            }
            continue;
        }

        r = &E.row[filerow];

        int len = r->rsize - E.coloff;
        int current_color = -1;
        if (len > 0) {
            if (len > E.screencols) len = E.screencols;
            char *c = r->render+E.coloff;
            unsigned char *hl = r->hl+E.coloff;
            int j;
            for (j = 0; j < len; j++) {
                if (hl[j] == HL_NONPRINT) {
                    char sym;
                    abAppend(&ab,"\x1b[7m",4);
                    if (c[j] <= 26)
                        sym = '@'+c[j];
                    else
                        sym = '?';
                    abAppend(&ab,&sym,1);
                    abAppend(&ab,"\x1b[0m",4);
                } else if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        abAppend(&ab,"\x1b[39m",5);
                        current_color = -1;
                    }
                    abAppend(&ab,c+j,1);
                } else {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color) {
                        char buf[16];
                        int clen = snprintf(buf,sizeof(buf),"\x1b[%dm",color);
                        current_color = color;
                        abAppend(&ab,buf,clen);
                    }
                    abAppend(&ab,c+j,1);
                }
            }
        }
        abAppend(&ab,"\x1b[39m",5);
        abAppend(&ab,"\x1b[0K",4);
        abAppend(&ab,"\r\n",2);
    }

    /* Create a two rows status. First row: */
    abAppend(&ab,"\x1b[0K",4);
    abAppend(&ab,"\x1b[7m",4);
    char status[80], rstatus[80], hstatus[5];
    int hlen = snprintf(hstatus,5,*E.syntax->filematch+1);
    if (hlen == 0) snprintf(hstatus,5,"text");
    int len = snprintf(status, sizeof(status), "%.20s - %d lines, highlight:%s %s",
        E.filename, E.numrows, hstatus, E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus),
        "%d/%d",E.rowoff+E.cy+1,E.numrows);
    if (len > E.screencols) len = E.screencols;
    abAppend(&ab,status,len);
    while(len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(&ab,rstatus,rlen);
            break;
        } else {
            abAppend(&ab," ",1);
            len++;
        }
    }
    abAppend(&ab,"\x1b[0m\r\n",6);

    /* Second row depends on E.statusmsg and the status message update time. */
    abAppend(&ab,"\x1b[0K",4);
    int msglen = strlen(E.statusmsg);
    if (msglen && time(NULL)-E.statusmsg_time < 5)
        abAppend(&ab,E.statusmsg,msglen <= E.screencols ? msglen : E.screencols);

    /* Put cursor at its current position. Note that the horizontal position
     * at which the cursor is displayed may be different compared to 'E.cx'
     * because of TABs. */
    int j;
    int cx = 1;
    int filerow = E.rowoff+E.cy;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    if (row) {
        for (j = E.coloff; j < (E.cx+E.coloff); j++) {
            if (j < row->size && row->chars[j] == TAB) cx += (TABSPACE-1)-((cx)%TABSPACE);
            cx++;
        }
    }
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",E.cy+1,cx);
    abAppend(&ab,buf,strlen(buf));
    abAppend(&ab,"\x1b[?25h",6); /* Show cursor. */
    write(STDOUT_FILENO,ab.b,ab.len);
    abFree(&ab);
}

/* Set an editor status message for the second line of the status, at the
 * end of the screen. */
void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(E.statusmsg,sizeof(E.statusmsg),fmt,ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

/* =============================== Find mode ================================ */
void editorFind(int fd, int mode) {
    char query[KILO_QUERY_LEN+1] = {0};
    int qlen = 0;
    char target[KILO_QUERY_LEN+1] = {0};
    int target_len = 0;
    int last_match = -1; /* Last line where a match was found. -1 for none. */
    int find_next = 0; /* if 1 search next, if -1 search prev. */
    int saved_hl_line = -1;  /* No saved HL */
    char *saved_hl = NULL;
    int firstrun =1;
    int c;
    int eof = 0;
    char *additional = NULL; /* next match in same line */

#define FIND_RESTORE_HL do { \
    if (saved_hl) { \
        memcpy(E.row[saved_hl_line].hl,saved_hl, E.row[saved_hl_line].rsize); \
        free(saved_hl); \
        saved_hl = NULL; \
    } \
} while (0)

    /* Save the cursor position in order to restore it later. */
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;
    
    editorQueryString("Search for:",query); 
    if (query[0]=='\0') return;
    qlen=strlen(query);
    if (mode!=0) {
    editorQueryString("Replace with:",target); 
    if (target[0]=='\0') return;
    target_len=strlen(target); }

    while(1) {
        if (mode != 1) {
            editorSetStatusMessage("Cursor down for next, Cursor up for previous, Enter to exit:");
        } else {
            editorSetStatusMessage("Home to replace, Cursor down for next, Cursor up for previous, Enter to exit:");
        }       
        editorRefreshScreen();

        if (firstrun==1) {
            firstrun=0;
            find_next = 1;        
        } else {
        if (mode == 2) { c = HOME_KEY; } 
        else { c = editorReadKey(fd); }
        if (c == DEL_KEY || c == CTRL_H || c == DELETE) {
            if (qlen != 0) query[--qlen] = '\0';
            last_match = -1;
        } else if (c == ESC || c == ENTER) {
            if (c == ESC) {
                E.cx = saved_cx; E.cy = saved_cy;
                E.coloff = saved_coloff; E.rowoff = saved_rowoff;
            }
            FIND_RESTORE_HL;
            editorSetStatusMessage("");
            return;
        } else if (c == ARROW_RIGHT || c == ARROW_DOWN) {
            find_next = 1;
        } else if (c == ARROW_LEFT || c == ARROW_UP) {
            find_next = -1;
        } else if (c == HOME_KEY) {
            if (mode != 0) {
              for(int j=0; j<qlen; j++) {
                editorMoveCursor(ARROW_RIGHT);
				editorDelChar(query[j]);
			  }            
              for(int j=0; j<target_len; j++) {
				editorInsertChar(target[j]);
			  } 
			  find_next = 1;
            }

        } else if (isprint(c & 255)) {
            if (qlen < KILO_QUERY_LEN) {
                query[qlen++] = c;
                query[qlen] = '\0';
                last_match = -1;
            }
        }
        } /* firstrun */

        /* Search occurrence. */
        if (last_match == -1) find_next = 1;
        if (find_next) {
            char *match = NULL;
            int match_offset = 0;
            int i, current = last_match;

            if (additional != NULL && find_next != -1) {
                /* todo: handle multiple matches in last line*/
                match = additional;
                additional = strstr(match+1,query);
                match_offset = match-E.row[current].render;
            } else {
                
            for (i = 0; i < E.numrows; i++) {
                current += find_next;
                if (current == E.numrows) eof=1; 
                if (current == -1) current = E.numrows-1;
                else if (current == E.numrows) current = 0;
                
                if (!(eof==1 && mode==2)){
                match = strstr(E.row[current].render,query);
                additional = strstr(match+1,query);
                if (match) {
                    match_offset = match-E.row[current].render;
                    break;
                }
                }
            }
            } /*additional*/
            find_next = 0;

            if (eof == 1) { /*no restart at top*/
                FIND_RESTORE_HL;
                if (mode == 2) {
                    E.cx = saved_cx; E.cy = saved_cy;
                    E.coloff = saved_coloff; E.rowoff = saved_rowoff;
                    editorSetStatusMessage(
                    "All occurrences replaced. Search function terminated.");
                    editorRefreshScreen();
                    return;
                }
            }
            eof=0;

            /* Highlight */
            FIND_RESTORE_HL;

            if (match) {
                erow *row = &E.row[current];
                last_match = current;
                if (row->hl) {
                    saved_hl_line = current;
                    saved_hl = malloc(row->rsize);
                    memcpy(saved_hl,row->hl,row->rsize);
                    memset(row->hl+match_offset,HL_MATCH,qlen);
                    memset(row->hl+match_offset+qlen,0,1); /*will remove any attribute */
                }
                E.cy = 0;
                E.cx = match_offset;
                E.rowoff = current;
                E.coloff = 0;
                /* Scroll horizontally as needed. */
                if (E.cx > E.screencols) {
                    int diff = E.cx - E.screencols;
                    E.cx -= diff;
                    E.coloff += diff;
                }
            }
        }
    } /* while */
}

/* ========================= Editor events handling  ======================== */

int modify_warning(int fd) {
        int c;
        if (!E.dirty) return 0;
        
        editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                "Enter 'y' to discard the changes:");
        editorRefreshScreen();
        c = editorReadKey(fd);
        editorSetStatusMessage("");
        editorRefreshScreen();
        if (c != 'y') { return -1; }
        return 0; /* y */
}

/* Handle cursor position change because arrow keys were pressed. */
void editorMoveCursor(int key) {
    int filerow = E.rowoff+E.cy;
    int filecol = E.coloff+E.cx;
    int rowlen;
    erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow];

    switch(key) {
    case ARROW_LEFT:
        if (E.cx == 0) {
            if (E.coloff) {
                E.coloff--;
            } else {
                if (filerow > 0) {
                    E.cy--;
                    E.cx = E.row[filerow-1].size;
                    if (E.cx > E.screencols-1) {
                        E.coloff = E.cx-E.screencols+1;
                        E.cx = E.screencols-1;
                    }
                }
            }
        } else {
            E.cx -= 1;
        }
        break;
    case ARROW_RIGHT:
        if (row && filecol < row->size) {
            if (E.cx == E.screencols-1) {
                E.coloff++;
            } else {
                E.cx += 1;
            }
        } else if (row && filecol == row->size) {
            E.cx = 0;
            E.coloff = 0;
            if (E.cy == E.screenrows-1) {
                E.rowoff++;
            } else {
                E.cy += 1;
            }
        }
        break;
    case ARROW_UP:
        if (E.cy == 0) {
            if (E.rowoff) E.rowoff--;
        } else {
            E.cy -= 1;
        }
        break;
    case ARROW_DOWN:
        if (filerow < E.numrows) {
            if (E.cy == E.screenrows-1) {
                E.rowoff++;
            } else {
                E.cy += 1;
            }
        }
        break;
    }
    /* Fix cx if the current line has not enough chars. */
    filerow = E.rowoff+E.cy;
    filecol = E.coloff+E.cx;
    row = (filerow >= E.numrows) ? NULL : &E.row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen) {
        E.cx -= filecol-rowlen;
        if (E.cx < 0) {
            E.coloff += E.cx;
            E.cx = 0;
        }
    }
}

/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void editorProcessKeypress(int fd) {
    int times;
    int rf_number;
    static int overwrite_mode = 0;
    int i;
    int c = editorReadKey(fd);
    if(helpFlag){
        if (c != CTRL_A) { //or toggle will not work
            helpFlag = 0;
            editorRefreshScreen();
        }
    } 
    if(bmFlag){
        bmFlag = 0;
        editorRefreshScreen();
    } 
    if(rfFlag){
        rfFlag = 0;
        editorRefreshScreen();
    } 
    switch(c) {
    case CTRL_A:
        if(helpFlag){
            helpFlag = 0;
            } 
        else {
            helpFlag = 1;	
        } 	
	break;
    case INS_KEY:
        if(overwrite_mode){
            overwrite_mode = 0;
            } 
        else {
            overwrite_mode = 1;	
        } 	
	break;
    case CTRL_Y:    
#if 0        
        clearScreen();
        int freemem=checkAvailableMemory();
        editorSetStatusMessage("There are %d kbyte memory available now.",freemem);
        editorRefreshScreen();
#endif        
        break;
    case ENTER:         /* Enter */
		ht.cx = E.coloff+E.cx;
		ht.cy = E.rowoff+E.cy;        
		turnOffTracing();
		ht.len = 1;        
		ht.cx = E.coloff+E.cx;
		ht.cy = E.rowoff+E.cy;
        editorInsertNewline();
        break;
    case CTRL_C:        /* Ctrl-c */
        editorSetStatusMessage("Select the area to copy using the cursor keys and press Enter.");
        editorRefreshScreen();
        copybuffer();
        break;
    case CTRL_V:
        pastebuffer();
        break;
    case CTRL_X:
        copylines(1);	
        removeOneLine();
        break;
	case CTRL_Z:
		undo();
		break;
    case CTRL_Q:        /* Ctrl-q */
        if (modify_warning(fd) == -1) return;
        saverecentfiles(); /*before clear screen*/
        /* move cursor below editor before quitting */
        char buf[32];
        struct abuf ab = ABUF_INIT;
        snprintf(buf,sizeof(buf),"\x1b[39m\x1b[%d;%dH\r\n\x1b[K",E.screenrows+1,1); //numrows
        abAppend(&ab,buf,strlen(buf));
        write(STDOUT_FILENO,ab.b,ab.len);
        abFree(&ab);
        clearScreen();
        exit(0);
        break;
    case CTRL_S:        /* Ctrl-s */
        editorSave();
        break;
    case CTRL_W:        /* Ctrl-w */
        editorSaveAs();
        break;
    case CTRL_P:        /* Ctrl-p buffer to disk*/
        bufferToFile();
        break;
    case CTRL_O:        /* Ctrl-o */
        if (modify_warning(fd) == -1) return;
        add_to_recentfiles(); /*save loaded file*/
        rf[0].recentfilename[0] = '\0'; /*indicate empty first entry*/
        editorOpenNew();
        break;
    case CTRL_N:        /* Ctrl-n */
        if (modify_warning(fd) == -1) return;
        add_to_recentfiles(); /*save loaded file*/
        rf[0].recentfilename[0] = '\0'; /*indicate empty first entry*/
        /* move to top of file */
	    E.cx = 0; E.coloff = 0;
	    E.cy = 0; E.rowoff = 0;
        storeRecentCurPos();
        erow *row;
        for (i=0;i<E.numrows;i++) {
            row = E.row+i;
            editorFreeRow(row);
        }
        initEditor();
        editorSelectSyntaxHighlight("unknown.txt");
        editorOpen("unknown.txt");
        editorSetStatusMessage("");
        break;
    case CTRL_L:        /* Ctrl-l */
        editorInsertFile(E.rowoff+E.cy);
        break;
    case CTRL_B:  
        editorSetStatusMessage("Enter bookmark number to set, select or delete (1-7). Or 'd' to display them:");
        editorRefreshScreen();
        c = editorReadKey(fd);
        editorBookmarks(fd,c-48); /* convert ASCII code to number */
		break;
    case CTRL_J:
        if(rfFlag){
            rfFlag = 0;
            } 
        else {
            rfFlag = 1;	
          if ((rf[0].recentfilename[0] != '\0') || (strcmp(E.filename,"unknown.txt") == 0)) {            
            editorSetStatusMessage("Enter the number of the file to load:");
            editorRefreshScreen();
            c = editorReadKey(fd);
            if(c == ESC || c == CTRL_J) {
                editorSetStatusMessage("");
                rfFlag=0;
                break;
            }
            rf_number=c-49; //zero based
            rfFlag = 0;

            if (strcmp(E.filename,"unknown.txt") != 0) {
            add_to_recentfiles(); /*save loaded file*/
            rf[0].recentfilename[0] = '\0'; /*indicate empty first entry*/
            rf_number++; //add_to_recentfiles pushed entries down
            }
            for (i=0;i<E.numrows;i++) {
                row = E.row+i;
                editorFreeRow(row);
            }
            initEditor();
            editorSelectSyntaxHighlight(rf[rf_number].recentfilename);
            editorOpen(rf[rf_number].recentfilename);
            editorSetStatusMessage("");
            loadRecentCurPos(rf_number);
          }
        } 	
	break;
    case CTRL_F:
        editorFind(fd,0);
        break;
    case CTRL_R:
        editorSetStatusMessage("Prompt to replace (enter 1) or replace all (enter 2):");
        editorRefreshScreen();
        c = editorReadKey(fd);
        if (c != '2') { editorFind(fd,1); }/*prompt to replace mode*/
        if (c == '2') { editorFind(fd,2); }/*replace all mode*/
		break;
    case CTRL_H:        /* Ctrl-h */
        editorDelChar();
		break;
    case DELETE:        /* Delete */
    case DEL_KEY:
		editorMoveCursor(ARROW_RIGHT);
        editorDelChar();
        break;
    case CTRL_E: /* Move to End of file */
	    E.cx = 0;
        E.coloff = 0;
	    E.cy = E.screenrows-1; 
        E.rowoff = E.numrows-E.screenrows;
        editorRefreshScreen();
        break;
    case CTRL_T: /* Move to Top of file */
	    E.cx = 0;
        E.coloff = 0;
	    E.cy = 0;
        E.rowoff = 0;
        break;
    case PAGE_UP:
    case PAGE_DOWN:
        if (c == PAGE_UP && E.cy != 0)
            E.cy = 0;
        else if (c == PAGE_DOWN && E.cy != E.screenrows-1)
            E.cy = E.screenrows-1;
        times = E.screenrows;
        while(times--)
            editorMoveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
        break;
    case HOME_KEY:
	    E.cx = 0;
        E.coloff = 0;
        break;
    case END_KEY:
        E.cx = E.row[E.rowoff+E.cy].size; 
        if (E.cx > E.screencols-1) {
            E.coloff = E.cx-E.screencols+1;
            E.cx = E.screencols-1;
        } 
        break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editorMoveCursor(c);
        break;
    case ESC:
        /* Nothing to do for ESC in this mode. */
        break;
    default:
        if (overwrite_mode && !(E.cx >= E.row[E.rowoff+E.cy].size) ) { 
            editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
        }
        editorInsertChar(c);
        break;
    }
}

int editorFileWasModified(void) {
    return E.dirty;
}

void copylines(int numlines) { 

    char* str=NULL;
    char buf[256];
    int i,len;
	int filerow = E.rowoff+E.cy;
    for (i=0;i<numlines;i++) {
      erow *row = (filerow >= E.numrows) ? NULL : &E.row[filerow+i];
      if (i==0) {
        str = (char*)malloc((sizeof(char)*row->size)+2); /*+2 for NL + zero byte */
        sprintf(buf,"%s\n",row->chars);
        strcpy(str,buf);
      } else {
        len = strlen(str);
        str = (char*)realloc(str,len+2+(sizeof(char)*row->size));
        sprintf(buf,"%s\n",row->chars);
        strcat(str,buf);
      }
    }   
	if(copyText)
		free(copyText);
	copyText = str;
}

void copybuffer(void) {

    char* str=NULL;
    char buf[256];
    int len, lineoffset;
	int filerow = E.rowoff+E.cy;
    int c,i;

    erow *row = (filerow >= E.numrows) ? NULL : &E.row[E.rowoff+E.cy];
    copyTopRow=E.rowoff+E.cy;
    copyBottomRow=copyTopRow; //init
    str = (char*)malloc((sizeof(char)*row->size)-E.cx+2); /*+2 for NL + zero byte */
    sprintf(buf,"%s\n",row->chars+E.cx);
    strcpy(str,buf);
    lineoffset = E.cx;
    memset(row->hl+E.cx,HL_MATCH,(sizeof(char)*row->size)-E.cx);
    
    while (1) {
      c = editorReadKey(0); 
      if (c == ARROW_RIGHT || c == ARROW_DOWN || c == ARROW_LEFT || c == ARROW_UP) {
        editorMoveCursor(c);
        editorRefreshScreen();
        if (c == ARROW_DOWN) {
            erow *row = (filerow >= E.numrows) ? NULL : &E.row[E.rowoff+E.cy];
            len = strlen(str);
            str = (char*)realloc(str,len+2+(sizeof(char)*row->size));
            sprintf(buf,"%s\n",row->chars);
            strcat(str,buf);
            memset(row->hl,HL_MATCH,(sizeof(char)*row->size));            
            lineoffset=0; /* indicates more than one line */
            copyBottomRow++; //to be able to clear
        }
      }
      if (c == ENTER) {
        if ((E.rowoff+E.cy < copyTopRow) || (lineoffset !=0 && E.cx < lineoffset)) { /* left or up from start point */
            for (i=copyTopRow; i < copyBottomRow+2; i++) {
                editorUpdateSyntax(&E.row[i]); 
            }
            free(str);
            copyTopRow = copyBottomRow = 0;
            return;
        }
        erow *row = (filerow >= E.numrows) ? NULL : &E.row[E.rowoff+E.cy];
        copyBottomRow = E.rowoff+E.cy+1;        
        if (lineoffset == 0) {
            /*arrow down saved full line already - remove buf of that*/
            snprintf(str,strlen(str)-strlen(buf)+1,"%s",str);
            len = strlen(str);
            str = (char*)realloc(str,len+2+E.cx);
            snprintf(buf,E.cx+1,"%s",row->chars);
            strcat(str,buf);
        } else {
            /* cut out chars behind cursor if just one line */
            snprintf(str,E.cx-1,"%s",str);
        }
        memset(row->hl+lineoffset,HL_MATCH,E.cx);  
        memset(row->hl+E.cx,0,(sizeof(char)*row->size)-E.cx); /*will remove any attribute */
        break;
      }
    }
       
	if(copyText)
		free(copyText);
	copyText = str;
}

void pastebuffer() {
    
    int i;
    
    if(copyText == NULL) 
		return;
	int size = strlen(copyText);

	for(int i=0; i<size; i++) {
        if (copyText[i] == '\n') {
            editorInsertNewline();
        } else {
            editorInsertChar(copyText[i]);
        }
	}
	if ((copyTopRow+copyBottomRow) != 0) {
        for (i=copyTopRow; i<copyBottomRow; i++) {
            editorUpdateSyntax(&E.row[i]);
        }
        copyTopRow = copyBottomRow = 0;
    }
}

int bufferToFile(void)
{
    if(copyText == NULL) 
		return -1;
    int fd, i;
	int size = strlen(copyText);
    char filenamestr[] = "kilobuffer.txt";
    
    unlink(filenamestr);
    fd = open(filenamestr,O_RDWR|O_CREAT,0644);
    if (fd == -1) return -1;

	for(i=0; i<size; i++) {
            if (write(fd,&copyText[i],1) != 1) return -1; //goto writeerr;
	}
    close(fd);
    /*erase highlight */
    if ((copyTopRow+copyBottomRow) != 0) {
        for (i=copyTopRow; i<copyBottomRow; i++) {
            editorUpdateSyntax(&E.row[i]);
        }
        copyTopRow = copyBottomRow = 0;
    }
    /* buffer is kept, just no highlight */
    editorSetStatusMessage("Copy buffer contents written to %s file on disk", filenamestr);
    return 0;
}


void updateWindowSize(void) {
    if (getWindowSize(STDIN_FILENO,STDOUT_FILENO,
                      &E.screenrows,&E.screencols) == -1) {
        perror("Unable to query the screen for size (columns / rows)");
        exit(1);
    }
    E.screenrows -= 2; /* Get room for status bar. */
}

void handleSigWinCh(int unused __attribute__((unused))) {
    updateWindowSize();
    if (E.cy > E.screenrows) E.cy = E.screenrows - 1;
    if (E.cx > E.screencols) E.cx = E.screencols - 1;
    editorRefreshScreen();
}

void initEditor(void) {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;
    helpFlag = 0;
}

int main(int argc, char **argv) {
    char mfilename[200];
    int result_open;
    if (argc != 2) {
        sprintf(mfilename,"unknown.txt");
    } else {
        sprintf(mfilename,"%s",argv[1]);
    }
    /* for undo feature */
    StackInit();
	HistoryInit();
    readrecentfiles();
    
    initEditor();
	updateWindowSize();
    signal(SIGWINCH, handleSigWinCh);
    editorSelectSyntaxHighlight(mfilename);
    result_open = editorOpen(mfilename);
    enableRawMode(STDIN_FILENO);    
    if (result_open == -1) {
        printf("\r\n\nInsufficient memory to load file reading line %d!\r\n",E.numrows);
        exit(-1);
    } else {
    editorSetStatusMessage(
        "Ctrl-A = help | Ctrl-Q = quit");
    }
    while(1) {
        editorRefreshScreen();
        editorProcessKeypress(STDIN_FILENO);
    }
    return 0;
}
