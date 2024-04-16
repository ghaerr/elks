/*
 *								xvi(H)
 *
 *	This program implements a screen oriented hexadecimal/octal editor
 *	whose commands are a subset of those of "vi".
 *
 *
 *	Author: B. Sartirana
 *
 *	History of changes:
 *
 *	85-08-30	BS		First implementation.
 *	85-09-12	BS		Added the capability to accept
 *					addresses with '.' and '$' as
 *					Ex does; fixed a lot of bugs;
 *					started the modifications to make
 *					the program follow the Unix
 *					Project Coding Standards.
 *
 *	85-09-13	BS		Removed bug consisting of possibility
 *					to issue ":!!" even if no shell
 *					command was previously given.
 *					Added command ":.=".
 *
 *	85-10-09	BS		Added the capability of octal
 *					displaying;
 *					made all screen parameters automatically
 *					set by means of termcap;
 *					modified display of quick reference
 *					for screens with 40 columns;
 *					added command "control-?" as alternative
 *					help request;
 *					added the programmability of page size.
 *
 *	85-11-20	BS		Made source code portable without any
 *					change on BSD and System V Unix.
 *
 *	86-02-19	BS		Implemented the possibility of
 *					interrupting a direct goto command
 *					by issuing a SIGINT signal;
 *					modified the command `G' in such a way
 *					the cursor is moved on the last byte
 *					of file (as vi does).
 *
 */

#include	<sys/types.h>
#include	<sys/ioctl.h>
#include        <termios.h>
/* #include	<sgtty.h>                                           tjp*/
#include	<ctype.h>
#include	<stdio.h>
#include	<sys/stat.h>
#include	<signal.h>
#include	<setjmp.h>

#define	OCTSTEP	4	/* step for cursor movement inside octal field */
#define	HEXSTEP	3	/* step for cursor movement inside hexadecimal field */
#define	ASCSTEP	1	/* step for cursor movement inside ascii field */

/* type of representation - the two following values are sensitive to display */
/* organization 							      */

#define	HEX	14
#define	OCT	17

/* return values of `mover' function -- they supply an indication of the new */
/* cursor position							     */

#define	XRXC	0
#define	XRLC	1
#define	LRXC	2
#define	LRLC	3

/* return values of `readtty' function */

#define	NORM	0	/* an ordinary character was typed in	*/
#define	MATCH	1	/* a match character was typed in	*/
#define	BS	'\b'	/* a backspace was typed in		*/

/* state which pattern `pattmatch' function has to search for	*/

#define	NEW	1	/* a new pattern has to be get from stdin	*/
#define	OLD	0	/* the last pattern got is that to be searched for */

/* define searching direction for `search' function	*/

#define	FORW	1
#define	BACK   	-1

#define	BLKSIZE	1024	/* general purpouse buffer size	*/

#define	FIRSTROW  4	/* first physical row of a field		 */

#define	XNBYTES	16	/* # of bytes per row displayed (hex representation) */
#define	ONBYTES	8	/* # of bytes per row displayed (oct representation) */

/* reply codes of most functions	*/

#define	OK	0
#define	STOP	1
#define	NOTOK	-1
#define QUIT    100

#define	NOFNAME	2	/* reply code of `wrnewf' function	*/

#define	ESC	0x1b
#define	BELL	7

/* some command codes	*/

#define	CTRL_L	0x0c	/* ctrl-l */
#define	CTRL_B	2
#define	CTRL_F	6
#define	CTRL_U	0x15
#define	CTRL_D	4
#define	CTRL_G	7
#define	HELP	0x1f	/* ctrl-? */

/* errors detected on file updating	*/

#define	WRITEERR	0
#define	INSERTERR	1

/* boolean values returned by many functions	*/

#define	FALSE	0
#define	TRUE	~FALSE

/* field identifiers	*/

#define	FIELD1	0	/* hex/oct field	*/
#define	FIELD2	1	/* ascii field		*/

/* insertion types	*/

#define	INSAFT	0	/* insert after cursor position		*/
#define	INSBEF	1	/* insert before cursor position	*/
#define	APPEOF	2	/* insert at end of file		*/

#ifndef FATT_RDONLY
#define	FATT_RDONLY	0
#endif

#ifndef FATT_WRONLY
#define	FATT_WRONLY	1
#endif

#ifndef FATT_RDWR
#define	FATT_RDWR	2
#endif

#ifndef FSEEK_ABSOLUTE
#define	FSEEK_ABSOLUTE	0
#endif

#ifndef FSEEK_RELATIVE
#define	FSEEK_RELATIVE	1
#endif

#ifndef FSEEK_EOF
#define	FSEEK_EOF	2
#endif

static char *qref_id[] = {

	"INFOS DISPLAYING COMMANDS",
	"-------------------------",
	"",
	"control-g  display file size expressed",
	"	    in the current numeric base",
	"	    and in decimal, plus cursor",
	"	    current address",
	"",
	"control-?  display this guide"
};

static char *qref_ec[] = {

	"EX-LIKE COMMANDS",
	"----------------",
	"",
	":x	save file and quit",
	"",
	":wq	save file and quit",
	"",
	":q[!]	quit (\"q!\" quits even",
	"	if some changes were not",
	"	saved)",
	"",
	":w[!] [filename]",
	"	save file; if \"filename\"",
	"	is supplied, the file is saved",
	"	into it; \"w!\" overrides write",
	"	protection",
	"",
	":r filename",
	"	insert a file after cursor",
	"	position",
	"",
	":f filename",
	"	change name of editing file",
	"",
	":<address>  display page beginning at",
	"	     <address>",
	"",
	":<address>,<address> d",
	"	delete all between the two",
	"	addresses",
	"",
	":<address>,<address> w[!] [filename]",
	"	save all between the two",
	"	addresses",
	"",
	":.=	display cursor current address",
	"",
	":!<shell command>",
	"	call shell to exec a command",
	"",
	":!!	exec last shell command issued",
	":h     display this menu",
	"",
	":set base=8/16",
	"	change numeric base",
	"",
	":set pagesz=n",
	"	set # of bytes per page",
	"",
	":set all",
	"	display current settings",
	"",
	"NOTE. <address> can be a decimal number",
	"or, if it begins with \"0x\", a hex number;",
	"if it begins with \"0\" not followed by",
	"\"x\", it's intended to be an octal number.",
	"In addition, it can be expressed by",
	"means of symbols \"$\" and \".\": \"$\"",
	"means \"end of file\", whilst \".\" means",
	"\"cursor current address\".",
	"",
	"Symbols between square brackets represent",
	"options."

};

static char *qref_ps[] = {

	"PATTERN SEARCHING COMMANDS",
	"--------------------------",
	"",
	"/ <pattern>  search <pattern> forward",
	"",
	"? <pattern>  search <pattern> backward",
	"",
	"n            search last pattern entered",
	"	      in the same direction",
	"",
	"N	      search last pattern entered",
	"	      in the opposite direction"
};

static char *qref_fu[] = {

	"FILE UPDATING COMMANDS",
	"----------------------",
	"",
	"r <byte>	replace 1 byte with <byte>",
	"		at cursor position",
	"",
	"R <text> ESC	replace 1 or more bytes",
	"		with <text> from cursor",
	"		position",
	"",
	"i <text> ESC	insert <text> before",
	"		cursor position",
	"",
	"a <text> ESC	insert <text> after",
	"		cursor position",
	"",
	"A <text> ESC	append <text> to end",
	"		of file",
	"",
	"x		delete the byte cursor",
	"		is on",
	"",
	"NOTE. To enter codes normally",
	"interpreted, make them preceded by",
	" a \"\\"
};

static char *qref_fd[] = {

	"FILE DISPLAYING COMMANDS",
	"------------------------",
	"",
	"control-f, f	display next page",
	"",
	"control-b, b	display preceding page",
	"",
	"control-u	scroll up half a page",
	"",
	"control-d	scroll down half a page",
	"",
	"control-l	redraw current page",
	"",
	"<address>G	display page beginning",
	"		at <address>",
	"",
	"NOTE. <address> can be a decimal number",
	"or, if begins with \"0x\", a hex number;",
	"if it begins with \"0\" not followed by",
	"\"x\", it's intended to be an octal",
	"number."
};

static char *qref_cm[] = {

	"CURSOR MOVEMENT COMMANDS",
	"------------------------",
	"",
	"TAB		change field",
	"",
	"l, SPACE	move right",
	"",
	"h, BS		move left",
	"",
	"k, -		move up",
	"",
	"j, +		move down",
	"",
	"LF, RET		move to new line",
	"",
	"H		move to home",
	"",
	"M		move to middle of page",
	"",
	"L		move to bottom of page",
	"",
	"$		move to end of line",
	"",
	"^, |		move to beginning of line"
};

static char *qref[] = {

"	QUICK REFERENCE GUIDE TO XVI\n\r\n\r",
"Enter:\n\r",
"	1  to get Cursor Movement Commands\n\r",
"	2  to get File Displaying Commands\n\r",
"	3  to get File Updating Commands\n\r",
"	4  to get Pattern Searching Commands\n\r",
"	5  to get Ex-like Commands\n\r",
"	6  to get Infos Displaying Commands\n\r",
"	any other key to exit\n\r\n\r"
};

#define	MAXQREF	(sizeof(qref)/sizeof(*qref))	/* # of rows in qref	*/


static char *qref80[] =
{
"                          COMMAND SUMMARY",
" ",
"Cursor movement      Display control      File updating    String searching",
"---------------      ---------------      -------------    ----------------",
" ",
"TAB: change field    ctrl-f,f: next page  i: insert        /: search forw",
"BS,h: left           ctrl-b,b: prec.page  a: local append  ?: search backw",
"j,+: down            ctrl-u: scroll up    A: append to eof n: repeat search",
"k,-: up              ctrl-d: scroll down  x: delete           same direction",
"l,SPACE: right       ctrl-l: redraw       r: replace 1     N: repeat search",
"LF, RET: newline     <address>G: goto        byte             opposite dir.",
"H: home                                   R: replace 1 or",
"M: middle                                    more bytes",
"L: bottom                  Ex-like commands",
"$: end of line             ----------------",
"^,|: beginning of line     :q[!]                   quit",
"                           :w[!] [filename]        write entire file",
"                           :wq or :x               write and quit",
"Miscellaneous              :r filename             read and insert after",
"-------------                                      cursor position",
"                           :f filename             change file name",
"ctrl-g: display file       :<address>              goto",
"        size and current   :<address>,<address> d  delete block",
"        cursor position    :<address>,<address> w[!] [filename]   write block",
"ctrl-?: display this       :.=                     display current address",
"        summary            :!<shell command>       issue a shell command",
"                           :!!                     repeat last shell command",
"                                                   issued",
"                           :h                      display this summary",
"                           :set base=8/16          change numeric base",
"                           :set pagesz=n           set # of bytes per page",
"                           :set all                display current settings",
" ",
"Symbols between \"[\" and \"]\" mean options.",
"<address> is a decimal number or, if preceded by \"0x\", a hex number",
"or, if preceded by a \"0\" not followed by \"x\", an octal number.",
"For ex-like commands, <address> = '.' means address of the byte the cursor",
"is on, and <address> = '$' means address of the last byte in the file.",
"The insert,append,replace modes are closed by <esc> key.",
"To enter the file characters normally interpreted, make them preceded by \"\\\"."
};


#ifdef PC

#define	LIST	0
#define	BOOL	1
#define	RANGE	2

struct opt_s {

	char	shortname[5];
	char	longname[19];
	int	optlength;
	int	type;
	int	nvalues;
	char	*value[4];
	int	curvalue;
};

static struct opt_s	option[] = {

	{"ba", "base", 4, LIST, 2, {"8", "16"}},
	{"br", "bytes/row", 9, RANGE, 2, {"1", "999"}},
	{"hd", "header", 6, BOOL},
	{"nr", "nrows", 5, RANGE, 2, {"4", "999"}},
	{"nc", "ncolumns", 8, RANGE, 2, {"40", "999"}}
};
#endif


static char	*mesg[] = {

/*  0 */	"Updating ...",
/*  1 */	"No write since last change (:q! overrides)",
/*  2 */	"Missing filename",
/*  3 */	"No such file or directory",
/*  4 */	"0 bytes",
/*  5 */	"Permission denied",
/*  6 */	"\n\r[Hit return to continue]",
/*  7 */	"No previous command",
/*  8 */	"Incomplete shell escape command",
/*  9 */	"[No write since last change]\n\r",
/* 10 */	"\"%s\" Illegal filename",
/* 11 */	"Wrong option",
/* 12 */	"\"%s\": Not an editor command",
/* 13 */	"Current address is ",
/* 14 */	"Badly formed address",
/* 15 */	"Not that many bytes",
/* 16 */	"No address allowed",
/* 17 */	"Address exceedes file length",
/* 18 */	"First address is greater than second one",
/* 19 */	" fewer",
/* 20 */	"\"%s\" File exists - use \"w!\" to overwrite",
/* 21 */	"%cInterrupt",
/* 22 */	"Pattern not found",
/* 23 */	"[modified] ",
/* 24 */	"usage: %s [-o] filename\n\r",
/* 25 */	"%s: termcaps not found",
/* 26 */	"%s: unknown option \"%s\"",
/* 27 */	"%s: cannot create %s",
/* 28 */	"%s: cannot open %s",
/* 29 */	"%s: cannot create temporary file",
/* 30 */	"%s: cannot open temporary file",
/* 31 */	"%s: cannot create auxiliary file",
/* 32 */	"%s: cannot open auxiliary file",
/* 33 */	"[New file] ",
/* 34 */	"Invalid filename",
/* 35 */	"File exists - use \"w! %s\" to overwrite",
/* 36 */	"%c*** write error",
/* 37 */	"%c*** write error: no insertion done",
/* 38 */	"Permission denied\n\r",
/* 39 */	"Unacceptable value",
/* 40 */	"No previous pattern",
/* 41 */	"Mantain working files \"%s\" and \"%s\" ?"
};

static char	*pmask[] = {

	"%o",			/*  0 */
	"%1o   ",		/*  1 */
	"%03o",			/*  2 */
	"%03o ",		/*  3 */
	"%11lo:",		/*  4 */
	"%lo",			/*  5 */
	"page %o (%d dec)",	/*  6 */
	"",			/*  7 */
	"%x",			/*  8 */
	"%1x  ",		/*  9 */
	"%02x",			/* 10 */
	"%02x ",		/* 11 */
	"%8lx:",		/* 12 */
	"%lx",			/* 13 */
	"page %x (%d dec)",	/* 14 */
	"",			/* 15 */
	"%ld",			/* 16 */
};

/* structure defining a field	*/

struct t_field {
	int lr;		/* last row			*/
	int lconlr;	/* last col on last row		*/
	int fc;		/* first col			*/
	int lcmax;	/* max value for last col	*/
	int step;	/* step for cursor movement	*/
};


static struct	stat	info;
static struct	stat	*pinfo;
/* static struct	sgttyb	ttyb;                               tjp */
static struct   termios  ttyb;           /*                          tjp */
static struct   termios  ttybsave;       /*                          tjp */
static struct	t_field	field1;		/* oct/hex field		*/
static struct	t_field	field2;		/* ascii field			*/
static struct	t_field	curfield;	/* field containing the cursor	*/
static char	rbuffer[BLKSIZE];	/* read buffer			*/
static char	wbuffer[BLKSIZE];	/* write buffer			*/
static char	lastcmd[81] = "";	/* last shell command issued	*/
static char	pattern[XNBYTES];	/* pattern to be searched	*/
static int	patlen = 0;		/* pattern length		*/
static int	prevdir;		/* previous searching direction	*/
static int	statusline;		/* # of the status line		*/
static int	radix;			/* numeric base choosen		*/
static int	nbytes;			/* # of bytes/row to be displayed */
static int	pagesz;			/* # of bytes/page		*/
static int	pageno;			/* current page #		*/
static int	lastrow;		/* last row of a field		*/
static int	nrows;			/* # of rows of a field		*/
static int	maxrow;			/* # of screen rows		*/
static int	maxcol;			/* # of screen columns		*/
static int	(* digitok)();		/* address of function used to	*/
					/* check typed in digits	*/
static int	xb = 12;		/* start column of hex field	*/
static int	ob = 15;		/* start column of oct field	*/

static jmp_buf	env;		/* context for `longjmp' function	*/
static int firstdisplay;	/* flag allowing SIGINT signal to be	*/
				/* ignored on first `display' call	*/
/*  tjp
static int flag;		/* tty driver flags		        */
static int actpos;		/* cursor logical offset from home position */
static long curflength = 0L;	/* length of the file being edited	*/
static int actfield;		/* region (hex or ascci) into which the	*/
				/* cursor is				*/
static int nochange = TRUE;	/* status of the file (modified or not)	*/
static long base;		/* address in the file of the page	*/
				/* currently displayed			*/
static int l;			/* length of the last data read		*/
static int reply;		/* auxiliary var			*/
static int newfile;		/* boolean var indicating whether the 	*/
				/* file did exist or not		*/
static int hnum;		/* start value for column numbering	*/
static int inpfd = -1;		/* descriptor of the input file		*/
static int tmpfd = -1;		/* descriptor of the temporary file	*/
static int auxfd = -1;		/* descriptor of the auxiliary file	*/
static char inpfname[PATH_MAX];	/* path name of the input file		*/
static char tmpfname[19];	/* path name of the temporary file	*/
static char auxfname[19];	/* path name of the auxiliary file	*/
static int x,y;			/* physical cursor coordinates		*/
static int quit = 0;		/* flag used to exit the program	*/

/* termcap var.s begin */


char	clearbuf[100];
extern char	PC;
extern short	ospeed;

char	*tgetstr();
char	*tgetenv();

char	*cl;	/* clear screen			*/
char	*cm;	/* cursor motion		*/
char	*ce;	/* clear to end of line		*/
char	*cd;	/* clear to end of display	*/

/* termcap var.s end */



/* list of local functions	*/

static void backward();
static void changefield();
static void cleareos();
static void copyblk();
static void curcoords();
static void delete();
static int det0();
static int det1();
static void display();
static void error();
static int esc_match();
static int forward();
static void getcmd();
static int gettcap ();
static void gotoaddr();
static void help();
static void help80();
static void insert();
static void insf();
#ifndef CBREAK
static void intdis();
static void inten();
#endif
static int intrproc();
static int isaddress();
static int ishex();
static int isoct();
static int isvalidname();
static void jmpproc();
static void lastpage();
static void logbegrow();
static void logbottom();
static void logendrow();
static void logeop();
static void loghome();
static void logmiddle();
static int min();
static void moved();
static void movel();
static int mover();
static void moveu();
static void mreplace();
static void mvcur();

#ifndef CBREAK
static int mygetchar();
#endif

static int myisdigit();
static int myisspace();
static int nevermatch();
static void newline();
static void pattmatch();
static void prepline();
static void printhdr();
static void printinfo();
static void printsize();
static void prpageno();
static void putch();
static int readtty();
static void redraw();
static void resterm();
static void scrolldown();
static void scrollup();
static int scrrefresh();
static long search();
static void b_select();
#ifdef PC
static void setparam();
#endif
static int setterm();
static int spec_match();
static void sreplace();
static int startup ();
static int stoln();
static void stopignore();
static void stopproc();
static void terminate();
static void toeosrefresh();
static int wrnewf();
static int wrsamef();

/*
 *								CUROFFS(M)
 *	NAME
 *		CUROFFS -- Compute cursor offset.
 *
 *	SYNOPSIS
 *		CUROFFS(r, c)
 *		int r, c;
 *
 *	DESCRIPTION
 *		This macro computes the offset of cursor position (r, c)
 *		(where 'r' stands for row and 'c' for column) from the home
 *		position. Note that this offset is intended as logical:
 *		it's the distance of a file pointer relative to the beginnig
 *		of current page.
 *
 *	RETURN VALUE
 *		It returns a value between 0 and page size less 1.
 *
 */

#define	CUROFFS(r,c)	(((r) - FIRSTROW) * nbytes + ((c) - curfield.fc) / curfield.step)

/*
 *								CUROUTSIDE(M)
 *	NAME
 *		CUROUTSIDE -- Check whether cursor is outside the active field.
 *
 *	SYNOPSIS
 *		CUROUTSIDE
 *
 *	DESCRIPTION
 *		This macro checks whether the cursor position (y, x) falls
 *		inside the active field (hex/oct or ascii).
 *
 *	RETURN VALUE
 *		It returns a boolean value.
 *
 */

#define	CUROUTSIDE ((y > curfield.lr) || (y == curfield.lr && x > curfield.lconlr))

/*
 *								GETACHAR(M)
 *	NAME
 *		GETACHAR -- Select a particular function to read stdin.
 *
 *	SYNOPSIS
 *		GETACHAR
 *
 *	DESCRIPTION
 *		This macro is used to select internal `mygetchar()' or
 *		library `getchar()' function to perform reading stdin,
 *		depending on the flags of tty driver available.
 *
 *	RETURN VALUE
 *		None.
 *
 */

#ifdef CBREAK
#define	GETACHAR	getchar()
#else
#define	GETACHAR	mygetchar()
#endif

#ifndef CBREAK
#define INTEN	inten()
#define INTDIS	intdis()
#else
#define INTEN
#define INTDIS
#endif

/*
 *								ISCTRL(M)
 *
 *	NAME
 *		ISCTRL -- check whether a char coding is in the ranges
 *			  00-1f or 7f-ff (hex).
 *
 *	SYNOPSIS
 *		ISCTRL(c)
 *		char c;
 *
 *	DESCRIPTION
 *		This macro is called with a character `c'. It checks
 *		whether the character belongs to the range 00-1f or 7f-ff (hex).
 *
 *	RETURN VALUE
 *		Boolean value.
 */

#define	ISCTRL(c) ((c) < 0x20 || (c) == 0x7f || ((c) & 0200))

/*
 *								MASK(M)
 *
 *	NAME
 *		MASK -- Compute an array index.
 *
 *	SYNOPSIS
 *		MASK(offset)
 *		int offset;
 *
 *	DESCRIPTION
 *		It computes the right index value for `pmask' array
 *		starting from the current numeric base (`radix' + 1) and
 *		the guide number `offset'.
 *
 *	RETURN VALUE
 *		This macro returns an index for `pmask' array.
 *
 */

#define	MASK(n) ((n) + radix - 7)

/*
 *								SETLASTROW(M)
 *	NAME
 *		SETLASTROW -- Set `lastrow' variable.
 *
 *	SYNOPSIS
 *		SETLASTROW
 *
 *	DESCRIPTION
 *		This macro sets `lastrow' variable.
 *
 *	RETURN VALUE
 *		None.
 *
 */

#define	SETLASTROW	lastrow = FIRSTROW  + nrows - 1;

/*
 *								backward(L)
 *	NAME
 *		backward -- Display preceding page.
 *
 *	SYNOPSIS
 *		void backward()
 *
 *	DESCRIPTION
 *		This function displays the preceding file page. If the
 *		current page is the first one, it outputs a bell signal.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void backward()

{

	if (base == 0) {

		/*
		** at the beginning of file
		*/

		putchar(BELL);
		return;
	}

	base -= pagesz;
	if (base < 0)
		base = 0L;
	scrrefresh(base);
	loghome();
}

/*
 *								changefield(L)
 *	NAME
 *		changefield -- Change active field.
 *
 *	SYNOPSIS
 *		void changefield(mode)
 *		int mode;
 *
 *	DESCRIPTION
 *		It makes the other field active. If `mode' is not zero, it
 *		moves cursor to the corresponding position in the new field.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void changefield(mode)

 int	mode;

{

	if (actfield == FIELD1) {

		/*
		** change to ascii field
		*/

		x = (x - field1.fc)/field1.step + field2.fc;
		actfield = FIELD2;
		curfield.fc = field2.fc;
		curfield.lcmax = field2.lcmax;
		curfield.lconlr = field2.lconlr;
		curfield.step = field2.step;
	}
	else {

		/*
		** change to hex/oct field
		*/

		x = (x-field2.fc)*field1.step + field1.fc;
		actfield = FIELD1;
		curfield.fc = field1.fc;
		curfield.lcmax = field1.lcmax;
		curfield.lconlr = field1.lconlr;
		curfield.step = field1.step;
	}

	if (mode)
		mvcur(y,x);
}

/*
 *								cleareos(L)
 *	NAME
 *		cleareos -- Clear screen from cursor position.
 *
 *	SYNOPSIS
 *		void cleareos()
 *
 *	DESCRIPTION
 *		It clears screen starting from cursor position.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void cleareos() {

	tputs(cd,1,putch);
}

/*
 *								copyblk(L)
 *	NAME
 *		copyblk -- Copy a portion of a file into an other file.
 *
 *	SYNOPSIS
 *		void copyblk(ifd, ofd, ifrom, ito, oto)
 *		int ifd, ofd;
 *		long ifrom, ito, oto;
 *
 *	DESCRIPTION
 *		It copies from file with descriptor `ifd' the block starting
 *		from `ifrom' and ending at `ito' into file  with descriptor
 *		`ofd' from `oto'. Note: it doesn't insert.
 *		No read/write error is detected.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void copyblk(ifd,ofd,ifrom,ito,oto)

 int	ifd,ofd;
 long	ifrom,ito,oto;

{
	lseek(ifd,ifrom,FSEEK_ABSOLUTE);
	lseek(ofd,oto,FSEEK_ABSOLUTE);
	while (ifrom <= ito)
		ifrom += write(ofd,rbuffer,min(read(ifd,rbuffer,BLKSIZE),ito-ifrom+1));
}

/*
 *								curcoords(L)
 *	NAME
 *		curcoords -- Compute cursor coordinates.
 *
 *	SYNOPSIS
 *		void curcoords(offs, yy, xx)
 *		int offs, *yy, *xx;
 *
 *	DESCRIPTION
 *		It computes screen cursor coordinates `yy' (row) and `xx'
 *		(column) corresponding to logical offset `offs'.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void curcoords(offs,yy,xx)

 int	offs,*yy,*xx;

{
	*xx = (offs % nbytes)*curfield.step + curfield.fc;
	*yy = offs / nbytes + FIRSTROW;
}

/*
 *								delete(L)
 *	NAME
 *		delete -- Delete one or more bytes.
 *
 *	SYNOPSIS
 *		void delete()
 *
 *	DESCRIPTION
 *		It deletes one or more bytes starting from cursor position.
 *		Physical deletion is done only when any command other than
 *		`delete' is given. Bytes deleted are substitued by `#'.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void delete() {

 int	pageback;
 int	oldactpos;
 int	i;
 char	c;
 long	index;
 int	eop;			/* End Of Page flag	*/
 int	x1,y1;
 long	startptr,tmpaddr,oldbase;

	if (curflength == 0L) {
		putchar(BELL);
		return;
	}
	ungetc('x',stdin);
	pageback = eop = FALSE;
	index = 0L;
	oldbase = base;
	startptr = tmpaddr = base + actpos;
	oldactpos = actpos;

	copyblk(tmpfd,auxfd,0L,tmpaddr-1,0L);

	x1 = x;
	y1 = y;

	do {
		if ((c = GETACHAR) != 'x')
			break;
		if (eop)
			if (forward() != OK)
				break;
			else
				eop = FALSE;
		tmpaddr++;
		index++;

		if (actfield == FIELD2) {
			putchar('#');
			mvcur(y,(x-field2.fc)*field1.step+field1.fc);
			if (radix == 15)
				printf("##");
			else
				printf("###");
		}
		else {
			if (radix == 15)
				printf("##");
			else
				printf("###");
			mvcur(y,(x-field1.fc)/field1.step+field2.fc);
			putchar('#');
		}

		i = actpos;
		if (mover() == LRLC)
			eop = TRUE;
		else
			actpos = i;
		mvcur(y,x);

	} while (tmpaddr < curflength);

	if (c != 'x')
		ungetc(c,stdin);

	if (tmpaddr < curflength) {

		mvcur(statusline,1);
		cleareos();
		printf(mesg[0]);
		fflush(stdout);

		copyblk(tmpfd,auxfd,tmpaddr,curflength-1,startptr);

		i = tmpfd;
		tmpfd = auxfd;
		auxfd = i;
	}
	else
		if (oldactpos == 0)
			pageback = TRUE;

	curflength -= index;

	nochange = FALSE;

	y = y1;
	x = x1;

	base = oldbase;
	if (pageback)
		if (base == 0)
			redraw();
		else {
			backward();
			logeop();
		}
	else {
		scrrefresh(base);
		if (CUROUTSIDE)
			logeop();
		else
			mvcur(y,x);
	}
}

/*
 *								det0(L)
 *	NAME
 *		det0 -- Check whether a character is NULL ('\0').
 *
 *	SYNOPSIS
 *		int det0(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is the NULL character.
 *
 *	RETURN VALUE
 *		It returns a boolean value.
 *
 */

static int det0(c)

 char	c;

{
	return(c == '\0');
}

/*
 *								det1(L)
 *	NAME
 *		det1 -- Check whether a character is comma, blank,
 *			TAB or NULL.
 *
 *	SYNOPSIS
 *		int det1(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is comma, blank, TAB or NULL.
 *
 *	RETURN VALUE
 *		It returns a boolean value.
 *
 */

static int det1(c)

 char	c;

{
	return(c == ',' || c == ' ' || c == '\t' || c == '\0');
}

/*
 *								display(L)
 *	NAME
 *		display -- Display a page of file.
 *
 *	SYNOPSIS
 *		void display(fr, addr)
 *		int fr;
 *		long addr;
 *
 *	DESCRIPTION
 *		It displays, accordingly to numeric base choosen, a page,
 *		or a portion of page, of input file. It starts from physical
 *		row `fr' and display addresses from `addr', until page
 *		fullfillment. Display process can be interrupted by generating
 *		a SIGINT signal.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void display(fr,addr)

 register int	fr;		/* first row to display */
 register long	addr;

{
 int	rem;
 int	rowcnt;
 int	i;
 int	j;
 int	n;
 int	lastrow;

	if (~firstdisplay) {
		INTEN;
		signal(SIGINT,jmpproc);
	}

	if (l == 0) {

		/* the file is empty */

		lastrow = curfield.lr = FIRSTROW;
		field1.lconlr = field1.fc;
		field2.lconlr = field2.fc;
		mvcur(FIRSTROW,1);
		printf(pmask[MASK(4)],0L);
		mvcur(FIRSTROW,field2.fc-1);
		printf("\"\"");
	}
	else {
		mvcur(fr,1);
		cleareos();
		if (rem = l%nbytes) {

			/*
			** rem <> 0
			*/

			lastrow = curfield.lr = l/nbytes + fr;
			field1.lconlr = field1.fc + (rem-1)*field1.step;
			field2.lconlr = field2.fc + rem - 1;
		}
		else {
			curfield.lr = l/nbytes + fr - 1;
			lastrow = curfield.lr + 1;
			field1.lconlr = field1.lcmax;
			field2.lconlr = field2.lcmax;
		}

		for (rowcnt=fr; rowcnt<lastrow; rowcnt++) {

			printf(pmask[MASK(4)],addr);
			mvcur(rowcnt,field1.fc);

			j = i = (rowcnt-fr) * nbytes;

			for (n = 1; n <= nbytes; n++)
				printf (pmask[MASK(3)],rbuffer[i++]&255);

			mvcur(rowcnt,field2.fc-1);
			putchar('"');
			for (n = 1; n <= nbytes;n++)
				if (ISCTRL(rbuffer[j])) {
					j++;
					putchar ('.');
				}
				else
					putchar (rbuffer[j++]);

			printf ("\"\n\r");

			addr += nbytes;
		}

		if ( rem != 0) {

			printf(pmask[MASK(4)],addr);
			mvcur(rowcnt,field1.fc);

			j = i = (rowcnt-fr) * nbytes;

			for (n = 1; n <= rem; n++)
				printf (pmask[MASK(3)],rbuffer[i++]&255);

			mvcur(rowcnt,field2.fc-1);
			putchar('"');

			for (n = 1; n <= rem; n++)
				if (ISCTRL(rbuffer[j])) {
					j++;
					putchar ('.');
				}
				else
					putchar (rbuffer[j++]);

			printf ("\"\n\r");

			addr += rem;
		}

	}

	if (actfield == FIELD1)
		curfield.lconlr = field1.lconlr;
	else
		curfield.lconlr = field2.lconlr;

	INTDIS;
	signal(SIGINT,intrproc);
}

/*
 *								error(L)
 *	NAME
 *		error -- Display error message.
 *
 *	SYNOPSIS
 *		void error(code)
 *		int code;
 *
 *	DESCRIPTION
 *		It displays an error message, depending on `code'.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void error(code)

 int	code;

{
	mvcur(statusline,1);
	cleareos();

	switch(code) {
	case WRITEERR:	printf(mesg[36],BELL);
			break;
	case INSERTERR:	printf(mesg[37],BELL);
			break;
	}

	mvcur(y,x);
}

/*
 *								c_index(L)
 *
 *	NAME
 *		index -- Compute the address of a character inside a string.
 *
 *	SYNOPSIS
 *		char *index(string, c)
 *		char *string;
 *		char c;
 *
 *	DESCRIPTION
 *		It computes the address of `c' inside `string'.
 *
 *	RETURN VALUE
 *		It returns 0 if doesn't find `c', otherwise its address.
 *
 */

char *c_index(string, c)

 char	*string;
 char	c;

{
	for (; *string != c && *string != '\0'; string++);
	if (*string == '\0')
		return((char *)0);
	else
		return(string);
}

/*
 *								esc_match(L)
 *	NAME
 *		esc_match -- Check whether a character is ESC or TAB.
 *
 *	SYNOPSIS
 *		int esc_match(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is the ESC or TAB character.
 *
 *	RETURN VALUE
 *		Boolean value.
 *
 */

static int esc_match(c)

 char	c;

{
	return(c == ESC || c == '\t');
}

/*
 *								forward(L)
 *	NAME
 *		forward -- Display next page.
 *
 *	SYNOPSIS
 *		int forward()
 *
 *	DESCRIPTION
 *		It displays the next file page, if there is one more.
 *
 *	RETURN VALUE
 *		It returns OK if a new page has been displayed, otherwise
 *		NOTOK.
 *
 */

static int forward() {


	if (curflength) {
		base += pagesz;
		if (scrrefresh(base) == -1) {
			base -= pagesz;
			return(NOTOK);
		}

		loghome();
		return(OK);
	}
	else
		return(NOTOK);
}

/*
 *								getcmd(L)
 *	NAME
 *		getcmd -- Read and execute an ex-like command.
 *
 *	SYNOPSIS
 *		void getcmd()
 *
 *	DESCRIPTION
 *		It reads and executes an ex-like command.
 *
 *	RETURN VALUE
 *		None.
 */

static void getcmd() {

 char	cmd[64];
 int	i,j;
 char	c;
 long	address,addr1,addr2;
 register char	*cptr;

	mvcur(statusline,1);
	cleareos();
	putchar(':');

	for (i = 0; i < 64; cmd[i++] = '\0');

	cmd[0] = GETACHAR;
	i = 0;
	while (cmd[i] != '\n' && cmd[i] != ESC) {
		if (cmd[i] == '\b')
			if (i == 0) {
				mvcur(y,x);
				return;
			}
			else {
				cmd[i] = '\0';
				cmd[i-1] = '\0';
				putchar('\b'); /* echo backspace */
				i -= 2;
			}
		else
			if (i < 63)
				putchar(cmd[i]);
			else {
				putchar(BELL);
				i--;
			}

		cmd[++i] = GETACHAR;
	}

	if (i == 0) {
		mvcur(y,x);
		return;
	}

	cmd[i] = '\0';
	mvcur(statusline,1);
	switch(cmd[0]) {

	case 'q':
		if (nochange || cmd[1] == '!')
			terminate();
		else {
			cleareos();
			printf(mesg[1]);
			mvcur(y,x);
		}
		break;

	case 'w':

		if ((reply = wrnewf(cmd,i,0L,curflength-1)) == NOFNAME) {
			reply = wrsamef(cmd,0L,curflength-1);
			if (reply == OK)
				nochange = TRUE;
			else
				if (reply == QUIT)
					break;
		}
		else
			if (reply == OK)
				nochange = TRUE;
		mvcur(y,x);
		break;

	case 'x':

		wrsamef(cmd,0L,curflength-1);
		break;

	case 'r':

		for (j = 1; j < i && isspace(cmd[j]); j++);
		if (j == i) {
			cleareos();
			printf(mesg[2]);
			mvcur(y,x);
		}
		else {
			cleareos();
			printf("\"%s\" ",&cmd[j]);
			fflush(stdout);
			if (stat(&cmd[j],pinfo) == -1) {
				printf(mesg[3]);
				mvcur(y,x);
				return;
			}
			if (pinfo->st_size == 0L)
				printf(mesg[4]);
			else
				if ((i = open(&cmd[j],FATT_RDONLY)) == -1) {
					printf(mesg[5]);
				}
				else {
					if (curflength == 0L) {
						copyblk(i,tmpfd,0L,pinfo->st_size-1,0L);
						curflength = pinfo->st_size;
					}
					else
						insf(i,base+actpos);
					printsize(pinfo->st_size,1);
					fflush(stdout);
					sleep(2);
					toeosrefresh(base+actpos-actpos%nbytes);
					/*
					mvcur(statusline,1);
					cleareos();
					printf("\"%s\" ",&cmd[j]);
					printsize(pinfo->st_size,1);
					*/
					close(i);
					nochange = FALSE;
				}
		}
		mvcur(y,x);
		break;

	case 'h':

		if (maxcol >= 80) {
			help80();
			printf(mesg[6]);
			GETACHAR;
			redraw();
		}
		else
			help();
		break;

	case '!':

		if (cmd[1] == '!')
			if (lastcmd[0] == '\0') {
				mvcur(statusline,1);
				cleareos();
				printf(mesg[7]);
				mvcur(y,x);
				return;
			}
			else
				strcat(lastcmd,&cmd[2]);
		else
			if (i == 1) {
				mvcur(statusline,1);
				cleareos();
				printf(mesg[8]);
				mvcur(y,x);
				return;
			}
			else
				strcpy(lastcmd,&cmd[1]);
		if (~nochange)
			printf(mesg[9]);

		resterm();
		putchar('\n');
		system(lastcmd);
		setterm();
		printf(mesg[6]);
		if ((c = GETACHAR) == ':')
			ungetc(c,stdin);
		else
			redraw();
		break;

	case 'f':

		for (j = 1; j < i && isspace(cmd[j]); j++);
		if (j == i) {
			cleareos();
			printf(mesg[2]);
		}
		else {
			if (! isvalidname(&cmd[j])) {
				cleareos();
				printf(mesg[10],&cmd[j]);
			}
			else {
				strcpy(inpfname,&cmd[j]);
				newfile = FALSE;
				printinfo();
			}
		}
		mvcur(y,x);
		break;

	case 's':
		if (strncmp(cmd,"set",3) == 0 ||
		    strncmp(cmd,"se",2)) {

			for (j = 3; isspace(cmd[j]) && j < i; j++);

			if (strncmp(&cmd[j],"base=",5) != 0 &&
			    strncmp(&cmd[j], "pagesz=",7) != 0 &&
			    strncmp(&cmd[j], "all", 3) != 0 || i == j) {
				cleareos();
				printf(mesg[11]);
				mvcur(y,x);
				return;
			}

			if (strncmp(&cmd[j], "base=", 5) == 0) {
				switch(cmd[j+5]) {
				case '8':
					b_select(OCT);
					break;
				case '1':
					if (cmd[j+6] == '6') {
						b_select(HEX);
						break;
					}
				default:
					cleareos();
					printf(mesg[39]);
					mvcur(y,x);
					return;
				}
				changefield(0);
				changefield(0);
			}
			else
				if (strncmp(&cmd[j], "pagesz=", 7) == 0) {
					i = atoi(&cmd[j+7]);
					if (i/nbytes > maxrow - FIRSTROW - 2 ||
				    	    i/nbytes <= 0) {
						mvcur(statusline,1);
						cleareos();
						printf(mesg[39]);
						mvcur(y,x);
						return;
					}
					nrows = i/nbytes;
					pagesz = nrows * nbytes;
					SETLASTROW;
				}
				else {
					/* :set all command	*/

					cleareos();
					printf("pagesz = ");
					printsize((long)pagesz, 0);
					printf("      base = %d", radix + 1);
					mvcur(y,x);
					return;
				}

			scrrefresh(base);
			loghome();
			break;
		}
		else {
			mvcur(statusline,1);
			cleareos();
			printf(mesg[12],cmd);
			mvcur(y,x);
			return;
		}

	default:

		if (!isaddress(cmd[0])) {

			mvcur(statusline,1);
			cleareos();
			printf(mesg[12],cmd);
			mvcur(y,x);
			return;
		}

		cptr = c_index(cmd,',');
		if (cptr == 0) {

			/* goto or ".=" command	*/

			switch(cmd[0]) {

			case '.':

					/* ".=" command	*/

				if (cmd[1] == '=') {
					cleareos();
					printf(mesg[13]);
					address = base + actpos;
					printsize(address,0);
				}
				mvcur(y,x);
				return;

			case '$':
				address = curflength - 1;
				break;

			default:
				/* goto command	*/

				if (stoln(cmd,&address,det0) == -1) {
				 	cleareos();
					printf(mesg[14]);
					mvcur(y,x);
					return;
				}
			}

			if (address >= base && (address - base) < pagesz && address < curflength) {
				actpos = (int) (address - base);
				curcoords(actpos, &y, &x);
				mvcur(y,x);
				return;
			}

			if (scrrefresh(address) == -1) {
				cleareos();
				printf(mesg[15]);
				mvcur(y,x);
				return;
			}
			base = address;
			loghome();
		}
		else {
			if (curflength == 0L) {
				cleareos();
				printf(mesg[16]);
				mvcur(y,x);
				return;
			}

			reply = 0;
			switch(cmd[0]) {

			case '.':
				if (!isspace(cmd[1]) && cmd[1] != ',')
					reply = -1;
				else
					addr1 = base + actpos;
				break;

			case '$':
				if (!isspace(cmd[1]) && cmd[1] != ',')
					reply = -1;
				else
					addr1 = curflength - 1;
				break;

			default:
				reply = stoln(cmd,&addr1,det1);
			}


			if (reply == -1) {
				cleareos();
				printf(mesg[14]);
				mvcur(y,x);
				return;
			}

			while (isspace(*++cptr));
			switch(*cptr) {

			case '.':
				addr2 = base + actpos;
				break;

			case '$':
				addr2 = curflength - 1;
				break;

			default:
				if (stoln(cptr,&addr2,myisspace) == -1) {
					cleareos();
					printf(mesg[14]);
					mvcur(y,x);
					return;
				}
			}

			while (!isspace(*cptr++));
			switch(*cptr) {

			case 'd':
			case 'w':
				if (addr2 > curflength ||
				addr1 > curflength) {
					cleareos();
					printf(mesg[17]);
					mvcur(y,x);
					return;
				}
				if (addr1 > addr2) {
					cleareos();
					printf(mesg[18]);
					mvcur(y,x);
					return;
				}

				if (*cptr == 'd') {
					nochange = FALSE;
					copyblk(tmpfd,auxfd,0L,addr1-1,0L);
					copyblk(tmpfd,auxfd,addr2+1,curflength-1,addr1);
					j = auxfd;
					auxfd = tmpfd,
					tmpfd = j;
					address = addr2 - addr1 + 1;
					curflength -= address;
					base = addr1 - 1;
					if (addr1 >= curflength) {
						backward();
						logeop();
					}
					else {
						if (base < 0L)
							base = 0L;
						scrrefresh(base);
						loghome();
					}
					mvcur(statusline,1);
					cleareos();
					printsize(address,1);
					printf(mesg[19]);
				}
				else {

					/* w case */

					for (i = 0; cptr[i] != '\0';i++);
					if (wrnewf(cptr,i,addr1,addr2) == NOFNAME)
						if (*(cptr+1) == '!')
							wrsamef(cptr,addr1,addr2);
						else {
							cleareos();
							printf(mesg[20],inpfname);
						}


				}
				mvcur(y,x);
				break;


			default:
				cleareos();
				printf(mesg[12],cmd);
				mvcur(y,x);
				return;
			}
		}
	}
}

/*
 *								gettcap(L)
 *	NAME
 *		gettcap -- Read terminal capabilities.
 *
 *	SYNOPSIS
 *		int gettcap()
 *
 *	DESCRIPTION
 *		It reads terminal capabilities `cd', `cm', `co', `li'.
 *
 *	RETURN VALUE
 *		It returns OK if termcap entry for terminal exists, otherwise
 *		NOTOK.
 *
 */

static int gettcap () {

 char	buf[1024];
 char	*clearptr;
 register char   *padstr;
 int	ldisc;

	if (tgetent(buf, getenv("TERM")) <= 0)
		return(NOTOK);

	clearptr = clearbuf;
	cd = tgetstr("cd", &clearptr);
	cm = tgetstr("cm", &clearptr);
	/*
	ce = tgetstr("ce", &clearptr);
	cl = tgetstr("cl", &clearptr);
	*/
	maxcol = tgetnum("co");
	maxrow = tgetnum("li");
	return(OK);
}

/*
 *								gotoaddr(L)
 *	NAME
 *		gotoaddr -- Display a page beginning at a given address.
 *
 *	SYNOPSIS
 *		void gotoaddr(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It gets an address from stdin whose first character is `c'
 *		read in "main" function, then it displays the page
 *		corresponding to this address.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void gotoaddr(c)

 char	c;

{

 char	address[12];
 long	actualaddr;
 int	i;

	address[0] = c;
	i = 1;
	for (;;) {
		if ((address[i] = GETACHAR) == 'G')
			break;
		else if (address[i] == 0x7f) {
			putchar(BELL);
			return;
		}
		if (i < 11)
			i++;
		else {
			putchar(BELL);
			return;
		}
	}

	address[i] = '\0';
	if (stoln(address,&actualaddr,det0) == -1)
		putchar(BELL);
	else
		if (actualaddr >= curflength)
			putchar(BELL);
		else
			if (actualaddr >= base && (actualaddr - base) < pagesz) {
				/* move cursor only	*/

				actpos = (int) (actualaddr - base);
				curcoords(actpos, &y, &x);
				mvcur(y,x);
			}
			else {
				base = actualaddr;
				scrrefresh(actualaddr);
				loghome();
			}
}

/*
 *								help(L)
 *	NAME
 *		help -- Display command summary on terminals with
 *		less than 80 columns.
 *
 *	SYNOPSIS
 *		void help()
 *
 *	DESCRIPTION
 *		It displays a menu driven command quick reference guide on
 *		terminals with less than 80 columns.
 *
 *	RETURN VALUE
 *		None.
 */

static void help() {

 int	i,j;
 int	max;
 char	c;
 register char	**ip;

	for (;;) {
		mvcur(1,1);
		cleareos();
		for (i = 0; i < sizeof(qref)/sizeof(*qref); i++)
			printf("%s\n\r",qref[i]);
		printf("choose: ");
		switch(GETACHAR) {

			case '1':
				ip = qref_cm;
				max = sizeof(qref_cm)/sizeof(*qref_cm);
				break;
			case '2':
				ip = qref_fd;
				max = sizeof(qref_fd)/sizeof(*qref_fd);
				break;
			case '3':
				ip = qref_fu;
				max = sizeof(qref_fu)/sizeof(*qref_fu);
				break;
			case '4':
				ip = qref_ps;
				max = sizeof(qref_ps)/sizeof(*qref_ps);
				break;
			case '5':
				ip = qref_ec;
				max = sizeof(qref_ec)/sizeof(*qref_ec);
				break;
			case '6':
				ip = qref_id;
				max = sizeof(qref_id)/sizeof(*qref_id);
				break;
			default:
				redraw();
				return;
		}
		mvcur(1,1);
		cleareos();
		j = 1;
		for (;;) {
			for (i = 1; i < statusline && j <= max; i++,j++)
				printf("%s\n\r",ip[j-1]);
			if (j > max) {
				printf("\n\rPRESS ANY KEY TO RETURN TO MENU");
				GETACHAR;
				break;
			}
			printf("MORE?");
			c = GETACHAR;
			if (c == 'n' || c == 'q')
				break;
			mvcur(i,1);
			cleareos();
		}
	}
}

/*
 *								help80()
 *	NAME
 *		help80 -- Display command summary on terminals with 80
 *			  or more columns.
 *
 *	SYNOPSIS
 *	 	void help80()
 *
 *	DESCRIPTION
 *		It displays a compact command quick reference guide on
 *		terminals with 80 or more columns.
 *
 *	RETURN VALUE
 *		None.
 */

static void help80() {

 int	i,j;
 char	c;

#define	MAXQR80		(sizeof(qref80)/sizeof(*qref80))

	mvcur(1,1);
	cleareos();
	j = 1;
	for (;;) {
		for (i = 1; i < statusline && j <= MAXQR80; i++,j++)
			printf("%s\n\r",qref80[j-1]);
		if (j > MAXQR80)
			return;
		printf("MORE?");
		c = GETACHAR;
		if (c == 'n' || c == 'q')
			return;
		mvcur(i,1);
		cleareos();
	}
}

/*
 *								insert(L)
 *	NAME
 *		insert -- Insert a text into a file.
 *
 *	SYNOPSIS
 *		void insert(mode, fd)
 *		int mode, fd;
 *
 *	DESCRIPTION
 *		This function inserts a text into a file whose descriptor
 *		is `fd'. Three insertion modes are supported: before cursor,
 *		after cursor and at end of file.
 *
 *	RETURN VALUE
 *		None.
 */

static void insert(mode,fd)

 int	mode;
 register int	fd;

{

 int	i;
 int	eoi,redrall;
 char	c;
 int	index;
 int	warning;
 long	startptr,tmpaddr,inslength;

	warning = redrall = eoi = FALSE;
	inslength = 0L;
	index = 0;

	if (mode != APPEOF) {
		if (mode == INSAFT) {
			if (mover() == LRLC) {
				warning = TRUE;
				y = FIRSTROW;
				x = curfield.fc;
				actpos = 0;
				base += pagesz;
				mvcur(y,1);
				cleareos();
				prepline(base);
			}
		}
		else
			if (y == curfield.lr && curfield.lconlr != curfield.lcmax) {
				mvcur(y,field2.lconlr+1);
				putchar(' ');
				mvcur(y,field2.lcmax+1);
				putchar('"');
			}
		startptr = tmpaddr = base + actpos;
		copyblk(tmpfd,auxfd,0L,tmpaddr-1,0L);
	}
	else {
		/* APPEOF */

		startptr = tmpaddr = curflength;
		lseek(tmpfd,curflength,FSEEK_ABSOLUTE);
		hnum = curflength%nbytes;
		printhdr();
		y = FIRSTROW;
		prepline(tmpaddr);
		x = curfield.fc;
		actpos = 0;
	}

	mvcur(y,x);

	while (~eoi)
		switch(readtty(&c,esc_match)) {

		case NORM:

			tmpaddr++;
			actpos++;
			inslength++;
			wbuffer[index++] = c;

			if (x < curfield.lcmax)
				x += curfield.step;
			else {
				if (y < lastrow) {
					y++;
					x = curfield.fc;
				}
				else {
					y = FIRSTROW;
					x = curfield.fc;
					actpos = 0;
				}
				prepline(tmpaddr);
			}

			mvcur(y,x);
			if (index == BLKSIZE) {
				if (write(fd,wbuffer,index) != index) {
					error(INSERTERR);
					return;
				}

				nochange = FALSE;
				curflength += index;
				index = 0;
			}
			break;

		case BS:

			putchar(BELL);
			break;

		case MATCH:

			if (c == '\t')
				changefield(1);
			else {
				if (inslength > 0L) {
					if (actpos == 0)
						actpos = pagesz - 1;
					else
						actpos--;
					if (x == curfield.fc) {
						if (y == FIRSTROW) {
							redrall = TRUE;
							y = lastrow;
						}
						else
							y--;
						x = curfield.lcmax;
					}
					else
						x -= curfield.step;
					mvcur(y,x);
				}

				eoi = TRUE;
			}
			break;
		}

	if (index != 0)
		if (write(fd,wbuffer,index) != index) {
			error(INSERTERR);
			return;
		}
		else {
			nochange = FALSE;
			curflength += index;
		}

	if (mode != APPEOF)
		if (inslength) {
			mvcur(statusline,1);
			cleareos();
			printf(mesg[0]);
			fflush(stdout);
			copyblk(tmpfd,auxfd,startptr,curflength-inslength-1,startptr+inslength);
			{
				i = auxfd;
				auxfd = tmpfd;
				tmpfd = i;

				lseek(auxfd,0L,FSEEK_ABSOLUTE);
			}
		}
		else {
			/* no char inserted */

			if (mode == INSAFT && warning) {
				base -= pagesz;
				scrrefresh(base);
				logeop();
			}
			else {
				mvcur(y,field2.lcmax+1);
				putchar(' ');
				if (y == curfield.lr)
					mvcur(y,field2.lconlr+1);
				else
					mvcur(y,field2.lcmax+1);
				putchar('"');
				mvcur(y,x);
			}
			return;
		}
	else
		/* mode was APPEOF */

		if (inslength) {

			curfield.lr = y;
			if (actfield == FIELD1) {
				curfield.lconlr = field1.lconlr = x;
				field2.lconlr = field2.fc + (x-field1.fc)/field1.step;
			}
			else {
				curfield.lconlr = field2.lconlr = x;
				field1.lconlr = (x-field2.fc)*field1.step + field1.fc;
			}
		}
		else {
			lastpage();
			return;
		}

	if (tmpaddr)
		base = --tmpaddr - actpos;

	if (redrall)
		scrrefresh(base);
	else
		toeosrefresh(tmpaddr - actpos%nbytes);
	mvcur(y,x);
}

/*
 *								insf(L)
 *	NAME
 *		insf -- Insert a file into working file.
 *
 *	SYNOPSIS
 *		void insf(inpfd, after)
 *		int infd;
 *		long after;
 *
 *	DESCRIPTION
 *		It inserts file whose descriptor is `inpfd' into working file
 *		after position `after'.
 *
 *	RETURN VALUE
 *		None.
 */

static void insf(inpfd,after)

 int	inpfd;
 long	after;

{
	copyblk(tmpfd,auxfd,0L,after,0L);
	copyblk(inpfd,auxfd,0L,pinfo->st_size-1,after+1);
	copyblk(tmpfd,auxfd,after+1,curflength-1,pinfo->st_size+after+1);
	inpfd = auxfd;
	auxfd = tmpfd;
	tmpfd = inpfd;
	curflength += pinfo->st_size;
}

/*
 *								intdis(L)
 *
 *	NAME
 *		intdis -- Disable interrupts.
 *
 *	SYNOPSIS
 *		void intdis()
 *
 *	DESCRIPTION
 *		It sets terminal to raw mode, so that no interrupt can
 *		be generated from keyboard.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void intdis() {

        return;                                                  /*  tjp */
/*      ttyb.sg_flags |= RAW;
	ioctl(2, TIOCSETP, &ttyb);                                       */
}

/*
 *								inten(L)
 *
 *	NAME
 *		inten -- Enable interrupts.
 *
 *	SYNOPSIS
 *		void inten()
 *
 *	DESCRIPTION
 *		It sets terminal to cooked mode, so that interrupts can
 *		be generated from keyboard.
 *
 *	RETURN VALUE
 *		None.
 *
 */

static void inten() {

        return;                                                  /*  tjp */
/*      ttyb.sg_flags &= ~RAW;
	ioctl(2, TIOCSETP, &ttyb);                                       */
}

/*
 *								intrproc(L)
 *	NAME
 *		intrproc -- Interrupt SIGINT service routine.
 *
 *	SYNOPSIS
 *		int intrproc()
 *
 *	DESCRIPTION
 *		This function serves the interrupt SIGINT. It allows
 *		to abort the editing session, loosing all the unsaved
 *		modifications. It can mantain the working files, if
 *		desired.
 *
 *	RETURN VALUE
 *		It returns -1 in the case of abort and nothing meaningful
 *		in the other cases.
 */

static int intrproc() {

	signal(SIGINT,intrproc);
	INTDIS;
	fflush(stdin);
	mvcur(statusline,1);
	cleareos();
	printf("%cABORT? ",BELL);
	if (GETACHAR == 'y') {
		mvcur(statusline, 1);
		cleareos();
		printf(mesg[41], tmpfname, auxfname);
		if (GETACHAR != 'n') {
			close(tmpfd);
			close(auxfd);
		}
		else
			terminate();

		resterm();
		exit(-1);
	}

	mvcur(y,x);
}

/*
 *								isaddress(L)
 *	NAME
 *		isaddress - Check whether a character is an `address' digit.
 *
 *	SYNOPSIS
 *		int isaddress(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether the character `c' is a digit or `.' or `$'.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int isaddress(c)

 char	c;

{
	return(isdigit(c) || c == '.' || c == '$');
}

/*
 *								ishex(L)
 *	NAME
 *		ishex -- Check whether a character is a hex digit.
 *
 *	SYNOPSIS
 *		int ishex(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is a hex digit.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int ishex(c)

 char	c;

{
	return((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

/*
 *								isoct(L)
 *	NAME
 *		isoct -- Check whether a character is an octal digit.
 *
 *	SYNOPSIS
 *		int isoct(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is a octal digit.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int isoct(c)

 char	c;

{
	return(c >= '0' && c <= '7');
}

/*
 *								isvalidname(L)
 *	NAME
 *		isvalidname -- Check the legality of a file name.
 *
 *	SYNOPSIS
 *		int isvalidname(string)
 *		char *string;
 *
 *	DESCRIPTION
 *		It checks the characters composing the file name `string'.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int isvalidname(string)

 char	*string;

{
	while (*string) {
		if (! isalnum(*string))
			switch(*string) {

			case '/':
			case '-':
			case '*':
			case '_':
			case '.':
			case '+':
			case '@':
			case '#':
			case '$':
			case '%':
				string++;
				break;
			default:
				return(FALSE);
			}
		else
			string++;
	}
	return(TRUE);
}

/*
 *								jmpproc(L)
 *	NAME
 *		jmpproc -- Procedure called to jump to main loop.
 *
 *	SYNOPSIS
 *		void jmpproc()
 *
 *	DESCRIPTION
 *		It's called by a SIGINT interrupt to return to command
 *		state, in main loop. It makes `intrproc' the current SIGINT
 *		interrupt service routine.
 *
 *	RETURN VALUE
 *		None.
 */

static void jmpproc() {

	INTDIS;
	mvcur(statusline,1);
	cleareos();
	printf(mesg[21],BELL);
	mvcur(y,x);
	signal(SIGINT,intrproc);
	longjmp(env,0);
}

/*
 *								lastpage(L)
 *	NAME
 *		lastpage -- Display the last page of working file.
 *
 *	SYNOPSIS
 *		void lastpage()
 *
 *	DESCRIPTION
 *		It displays the last page of working file. The cursor is
 *		moved onto the last byte.
 *
 *	RETURN VALUE
 *		None.
 */

static void lastpage() {

	if (pagesz > curflength)
		base = 0L;
	else
		base = curflength - pagesz;

	scrrefresh(base);

	logeop();
}

/*
 *								logbegrow(L)
 *	NAME
 *		logbegrow -- Move cursor at beginning of row.
 *
 *	SYNOPSIS
 *		void logbegrow()
 *
 *	DESCRIPTION
 *		It moves the cursor at the beginning of the row inside the
 *		active field.
 *
 *	RETURN VALUE
 *		None.
 */

static void logbegrow() {

	x = curfield.fc;
	mvcur(y,x);
	actpos = CUROFFS(y,x);
}

/*
 *								logbottom(L)
 *	NAME
 *		logbottom -- Move cursor onto last row, first column.
 *
 *	SYNOPSIS
 *		void logbottom()
 *
 *	DESCRIPTION
 *		It moves the cursor onto the last row, first column, inside
 *		the active field.
 *
 *	RETURN VALUE
 *		None.
 */

static void logbottom() {

	y = curfield.lr;
	x = curfield.fc;
	mvcur(y,x);
	actpos = CUROFFS(y,x);
}

/*
 *								logendrow(L)
 *	NAME
 *		logendrow -- Move cursor to end of row.
 *
 *	SYNOPSIS
 *		void logendrow()
 *
 *	DESCRIPTION
 *		It moves the cursor to the end of the row inside the active
 *		field.
 *
 *	RETURN VALUE
 *		None.
 */

static void logendrow() {


	if (y == curfield.lr)
		x = curfield.lconlr;
	else
		x = curfield.lcmax;
	mvcur(y,x);
	actpos = CUROFFS(y,x);
}

/*
 *								logeop(L)
 *	NAME
 *		logeop -- Move cursor to end of page.
 *
 *	SYNOPSIS
 *		void logeop()
 *
 *	DESCRIPTION
 *		It moves the cursor to the end of page, inside the active field.
 *
 *	RETURN VALUE
 *		None.
 */

static void logeop() {

	y = curfield.lr;
	x = curfield.lconlr;
	mvcur(y,x);
	actpos = CUROFFS(y,x);
}

/*
 *								loghome(L)
 *	NAME
 *		loghome -- Move cursor to home position.
 *
 *	SYNOPSIS
 *		void loghome()
 *
 *	DESCRIPTION
 *		It moves the cursor to home position, inside the active field.
 *
 *	RETURN VALUE
 *		None.
 */

static void loghome() {

	y = FIRSTROW;
	x = curfield.fc;
	mvcur(y,x);
	actpos = 0;
}

/*
 *								logmiddle(L)
 *	NAME
 *		logmiddle -- Move cursor to middle of page.
 *
 *	SYNOPSIS
 *		void logmiddle()
 *
 *	DESCRIPTION
 *		It moves the cursor to the middle of page, inside the active
 *		field.
 *
 *	RETURN VALUE
 *		None.
 */

static void logmiddle() {

	y = (curfield.lr-FIRSTROW)/2 + FIRSTROW;
	x = curfield.fc;
	mvcur(y,x);
	actpos = CUROFFS(y,x);
}

/*
 *								main(E)
 *	NAME
 *		main -- Main command dispatcher.
 *
 *	SYNOPSIS
 *		int main(argc, argv)
 *		int argc;
 *		char **argv;
 *
 *	DESCRIPTION
 *		It's the command dispatcher.
 *
 *	RETURN VALUE
 *		It returns 1 if initialization failes, otherwise 0.
 */

int main(argc,argv)

int	argc;
char	**argv;

{
	char	c;

	if (startup (argc,argv) != OK) {
		terminate();
		exit (1);
	}

	for (;!quit;) {
		setjmp(env);
		c=GETACHAR;

		switch(c) {

		case 'h':
		case '\b':
			movel();
			break;
		case 'j':
		case '+':
			moved();
			break;
		case 'k':
		case '-':
			moveu();
			break;
		case 'l':
		case ' ':
			if (mover() == LRLC)
				putchar(BELL);
			break;
		case '\n':
			newline();
			break;
		case '\t':
			changefield(1);
			break;
		case 'H':
			loghome();
			break;
		case 'L':
			logbottom();
			break;
		case 'M':
			logmiddle();
			break;
		case '|':
		case '^':
			logbegrow();
			break;
		case '$':
			logendrow();
			break;
		case 'r':
			sreplace();
			break;
		case 'R':
			mreplace();
			break;
		case 'f':
		case 'F':
		case CTRL_F:
			if (forward() != OK)
				putchar(BELL);
			break;
		case 'b':
		case 'B':
		case CTRL_B:
			backward();
			break;
		case 'i':
			if (curflength > 0L)
				insert(INSBEF,auxfd);
			else
				insert(APPEOF,tmpfd);
			break;
		case 'A':
			insert(APPEOF,tmpfd);
			break;
		case 'a':
			if (curflength == 0L || base+actpos+1 == curflength)
				insert(APPEOF,tmpfd);
			else
				insert(INSAFT,auxfd);
			break;
		case 'x':
			delete();
			break;
		case CTRL_L:
			redraw();
			break;
		case ':':
			getcmd();
			break;
		case 'G':
			lastpage();
			break;
		case CTRL_G:
			printinfo();
			break;
		case CTRL_U:
			scrollup();
			break;
		case CTRL_D:
			scrolldown();
			break;
		case '/':
			pattmatch(FORW,NEW);
			break;
		case '?':
			pattmatch(BACK,NEW);
			break;
		case 'n':
			pattmatch(prevdir,OLD);
			break;
		case 'N':
			pattmatch(-1*prevdir,OLD);
			break;
		case HELP:
			if (maxcol >= 80) {
				help80();
				printf(mesg[6]);
				GETACHAR;
				redraw();
			}
			else
				help();
			break;
		default:
			if (isdigit(c))
				gotoaddr(c);
			else
				putchar(BELL);

		}
	}

	resterm();
	exit(0);
}

/*
 *								min(L)
 *	NAME
 *		min -- Compute the minimum between two values.
 *
 *	SYNOPSIS
 *		int min(a, b)
 *		int a;
 *		long b;
 *
 *	DESCRIPTION
 *		It computes the minimun between the integer `a' and the
 *		long integer `b'.
 *
 *	RETURN VALUE
 *		It returns an integer.
 *
 */

 static int min(a, b)

  register int a;
  register long b;

{
	return((a < b) ? a : b);
}

/*
 *								moved(L)
 *	NAME
 *		moved -- Move cursor down.
 *
 *	SYNOPSIS
 *		void moved()
 *
 *	DESCRIPTION
 *		It moves the cursor one line down. An attempt to go beyond
 *		the last row generates a bell signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void moved() {

	if (y < curfield.lr - 1) {

		/*
		** regular case
		*/

		mvcur(++y,x);
		actpos += nbytes;
	}
	else
		if (y == curfield.lr - 1)

			/*
			** on the row before the last one
			*/

			if (x > curfield.lconlr)
				putchar(BELL);
			else {
				mvcur(++y,x);
				actpos += nbytes;
			}
		else
			/*
			** on the last row
			*/

			putchar(BELL);
}

/*
 *								movel(L)
 *	NAME
 *		moved -- Move cursor left.
 *
 *	SYNOPSIS
 *		void movel()
 *
 *	DESCRIPTION
 *		It moves the cursor left. An attempt to go beyond home
 *		position generates a bell signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void movel() {

	if (y == FIRSTROW)

		/*
		** on the first row
		*/

		if (x > curfield.fc) {

			/*
			** not on the first column
			*/

			x -= curfield.step;
			actpos--;
		}
		else
			/*
			** on the first column
			*/

			putchar(BELL);
	else
		/*
		** not on the first row
		*/

		if (x > curfield.fc) {

			/*
			** not on the first column
			*/

			x -= curfield.step;
			actpos--;

		}
		else {
			/*
			** on the first column
			*/

			y--;
			x = curfield.lcmax;
			actpos--;
		}

	mvcur(y,x);
}

/*
 *								mover(L)
 *	NAME
 *		moved -- Move cursor rigth.
 *
 *	SYNOPSIS
 *		int mover()
 *
 *	DESCRIPTION
 *		It moves the cursor rigth logically, without acting on the
 *		terminal.
 *
 *	RETURN VALUE
 *		It returns a code describing the new cursor position:
 *		XRXC:	any row/column other than the last one
 *		LRXC:	last row, any column except the last one
 *		LRLC:	last row, last column
 *		XRLC:	any row except the last one, last column
 *
 */

static int mover() {

	if (y == curfield.lr)

		/*
		** on the last row
		*/

		if (x < curfield.lconlr) {

			/*
			** not on the last column
			*/

			x += curfield.step;
			actpos++;
			mvcur(y,x);
			return(LRXC);
		}
		else
			/*
			** on the last column
			*/

			return(LRLC);
	else
		/*
		** not on the last row
		*/

		if (x < curfield.lcmax) {

			/*
			** not on the last column
			*/

			x += curfield.step;
			actpos++;
			mvcur(y,x);
			return(XRXC);
		}
		else {
			/*
			** on the last column
			*/

			y++;
			x = curfield.fc;
			actpos++;
			mvcur(y,x);
			return(XRLC);
		}
}

/*
 *								moveu(L)
 *	NAME
 *		moveu -- Move cursor up.
 *
 *	SYNOPSIS
 *		void moveu()
 *
 *	DESCRIPTION
 *		It moves the cursor one line up. An attempt to go beyond
 *		the first field's row generates a bell signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void moveu() {

	if (y > FIRSTROW) {

		/*
		** not on the first row
		*/

		mvcur(--y,x);
		actpos -= nbytes;
	}
	else
		putchar(BELL);
}

/*
 *								mreplace(L)
 *	NAME
 *		mreplace -- Replace a portion of file.
 *
 *	SYNOPSIS
 *		void mreplace()
 *
 *	DESCRIPTION
 *		It replaces a portion of working file. There is no limitation
 *		on the length of replaced text.
 *
 *	RETURN VALUE
 *		None.
 */

static void mreplace() {

 int	eoi,goback;
 char	c;
 int	index;
 int	replength;
 int	eof;
 long	offs;

	goback = eof = eoi = FALSE;
	replength = index = 0;
	offs = base + actpos;

	while (~eoi) {
		switch(readtty(&c,esc_match)) {

		case NORM:

			wbuffer[index++] = c;

			if (mover() == LRLC)
				if (forward() != OK) {
					mvcur(y,x);
					eof = TRUE;
					eoi = TRUE;
				}

			if (index == BLKSIZE) {
				lseek(tmpfd,offs,FSEEK_ABSOLUTE);
				if (write(tmpfd,wbuffer,index) != index) {
					error(INSERTERR);
					return;
				}

				offs += index;
				nochange = FALSE;
				replength += index;
				index = 0;
			}
			break;

		case BS:

			putchar(BELL);
			break;

		case MATCH:

			if (c == '\t')
				changefield(1);
			else {
				if (x == curfield.fc) {
					if (y > FIRSTROW) {
						y--;
						x = curfield.lcmax;
					}
					else
						if (index > 0)
							goback = TRUE;
				}
				else
					if ((y < curfield.lr || x != curfield.lconlr)
					    && (replength || index))
						x -= curfield.step;
				actpos = CUROFFS(y,x);
				mvcur(y,x);
				eoi = TRUE;
			}
			break;
		}
	} /* end while */

	if (index) {
		lseek(tmpfd,offs,FSEEK_ABSOLUTE);
		if (write(tmpfd,wbuffer,index) != index)
			error(WRITEERR);
		else
			nochange = FALSE;
		if (goback) {
			backward();
			logeop();
		}
	}

	if (eof)
		insert(APPEOF,tmpfd);
}

/*
 *								mvcur(L)
 *	NAME
 *		mvcur -- Move cursor to some position.
 *
 *	SYNOPSIS
 *		void mvcur(y, x)
 *		int y, x;
 *
 *	DESCRIPTION
 *		It moves cursor to position (y,x). Termcap refers coordinates
 *		to (0,0), whilst editor to (1,1), so a decrement is necessary.
 *
 *	RETURN VALUE
 *		None.
 */

static void mvcur(y,x)
 int  x,y;
{
 register char	*mov;
 char *tgoto();

        mov = tgoto(cm,x-1,y-1);
	tputs(mov,1,putch);
	fflush(stdout);
}

/*
 *								mygetchar(L)
 *
 *	NAME
 *		mygetchar -- Read a character from stdin.
 *
 *	SYNOPSIS
 *		int mygetchar()
 *
 *	DESCRIPTION
 *		It reads a character by mean of `getchar' function and
 *		takes the 7 l.s. bits of it. In addition, it translates
 *		CRs into LFs.
 *
 *	RETURN VALUE
 *		It returns the character read stored in an integer.
 *
 */

#ifndef CBREAK
static int mygetchar() {

 int c;

	return(((c = getchar() & 0x7f) == '\r') ? '\n' : c);
}
#endif

/*
 *								myisdigit(L)
 *	NAME
 *		myisdigit -- Check whether a character is a digit.
 *
 *	SYNOPSIS
 *		int myisdigit(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is a digit.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int myisdigit(c)

 char	c;

{
	return(isdigit(c));
}

/*
 *								myisspace(L)
 *	NAME
 *		myisspace -- Check whether a character is space or NULL.
 *
 *	SYNOPSIS
 *		int myisspace(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is a spece or NULL character.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int myisspace(c)

 char	c;

{
	return(isspace(c) || c == '\0');
}

/*
 *								nevermatch(L)
 *	NAME
 *		nevermatch -- Always return FALSE.
 *
 *	SYNOPSIS
 *		int nevermatch()
 *
 *	DESCRIPTION
 *		It is useful in `readtty' function when called by `sreplace'.
 *
 *	RETURN VALUE
 *		It always returns FALSE.
 */

static int nevermatch() {

	return(FALSE);
}

/*
 *								newline(L)
 *	NAME
 *		newline -- Move cursor to beginning of next line.
 *
 *	SYNOPSIS
 *		void newline()
 *
 *	DESCRIPTION
 *		It moves the cursor to the beginning of the next line.
 *		An attempt to go beyond the last line generates a bell
 *		signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void newline() {

	if (y != curfield.lr) {

		/*
		** not on the last row
		*/

		x = curfield.fc;
		mvcur(++y,x);
		actpos = CUROFFS(y,x);
	}
	else
		putchar(BELL);

}

/*
 *								pattmatch(L)
 *	NAME
 *		pattmatch -- Search for a pattern.
 *
 *	SYNOPSIS
 *		void pattmatch(direction, whitchpattern)
 *		int direction, whitchpattern;
 *
 *	DESCRIPTION
 *		If `whitchpattern' is equal to NEW, it gets a pattern from
 *		stdin, otherwise it gets that in the global variable `pattern',
 *		then it begins to search in the direction specified by
 *		`direction'. Searching is interruptable by generating SIGINT
 *		signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void pattmatch(direction,whichpattern)

 int	direction,whichpattern;

{

 int	eoi;
 int	i;
 int	y1,x1;
 char	c;
 char	curpattern[XNBYTES];
 long	b;
 int	oldfield;

	oldfield = actfield;
	y1 = y;
	x1 = x;
	x = curfield.fc;
	y = statusline;
	mvcur(y,1);
	cleareos();
	if (direction == FORW)
		putchar('/');
	else
		putchar('?');

	mvcur(y,1);

	if (whichpattern == NEW) {
		mvcur(y,field2.fc-1);
		printf("\"\"");
		mvcur(y,x);
		i = 0;
		eoi = FALSE;

		while (~eoi)
			switch(readtty(&c,spec_match)) {
			case NORM:
				if (x < curfield.lcmax) {
					curpattern[i++] = c;
					x += curfield.step;
					mvcur(y,i+field2.fc);
				}
				else {
					putchar(BELL);
					mvcur(y,field2.lcmax+1);
				}
				putchar('"');
				mvcur(y,x);
				break;

			case MATCH:
				switch(c) {
				case '\t':

					changefield(1);
					break;

				case '\n':
				case ESC:
					eoi = TRUE;
					break;
				}
				break;

			case BS:

				if (i) {
					mvcur(y,i+field2.fc-1);
					printf("\"  ");
					mvcur(y,field1.fc+(i-1)*field1.step);
					printf("       ");
					i--;
					x -= curfield.step;
					mvcur(y,x);
				}
				else
					eoi = TRUE;
				break;
			}
		if (i == 0) {
			if (patlen == 0) {
				mvcur(statusline, 1);
				cleareos();
				if (actfield != oldfield)
					changefield(0);
				y = y1;
				x = x1;
				mvcur(y,x);
				return;
			}
		}
		else {
			strncpy(pattern,curpattern,i);
			patlen = i;
		}

		prevdir = direction;
		mvcur(y,1);
	}
	else
		/* search for last pattern entered */
		if (patlen == 0) {
			printf(mesg[40]);
			if (actfield != oldfield)
				changefield(0);
			y = y1;
			x = x1;
			mvcur(y,x);
			return;
		}

	if (actfield != oldfield)
		changefield(0);
	y = y1;
	x = x1;
	INTEN;
	signal(SIGINT,jmpproc);

	for (;;)
		if ((b = search(base+actpos,pattern,patlen,direction)) >= 0)
			if (b >= base && (b-base) < pagesz) {

				/*
				** the occurrence is on this page
				*/

				actpos = (int)(b-base);
				curcoords(actpos,&y,&x);
				mvcur(y,x);
				break;
			}
			else
				if (b < base) {
					base = b - (pagesz - patlen);
					if (base < 0L)
						base = 0L;
					scrrefresh(base);
					actpos = (int)(b-base);
					curcoords(actpos,&y,&x);
					mvcur(y,x);
					break;
				}
				else {
					base = b;
					scrrefresh(b);
					loghome();
					break;
				}
		else {
			mvcur(statusline,1);
			cleareos();
			printf(mesg[22]);
			y = y1;
			x = x1;
			mvcur(y,x);
			break;
		}

	INTDIS;
	signal(SIGINT,intrproc);
}

/*
 *								prepline(L)
 *	NAME
 *		prepline -- Display a virgin line.
 *
 *	SYNOPSIS
 *		void prepline(tmpaddr)
 *		long tmpaddr;
 *
 *	DESCRIPTION
 *		It displays a virgin line to be filled with new text.
 *
 *	RETURN VALUE
 *		None.
 */

static void prepline(tmpaddr)

 long	tmpaddr;

{
	mvcur(y,1);
	printf(pmask[MASK(4)],tmpaddr);
	mvcur(y,field2.fc-1);
	putchar('"');
	mvcur(y,field2.lcmax+1);
	putchar('"');
}

/*
 *								printhdr(L)
 *	NAME
 *		printhdr -- Display page # and column numbering.
 *
 *	SYNOPSIS
 *		void printhdr()
 *
 *	DESCRIPTION
 *		It display the # of the page being showed and the column
 *		numbering for both fields.
 *
 *	RETURN VALUE
 *		None.
 */

static void printhdr() {

 int	i;

	mvcur(1,1);
	cleareos();
	prpageno();
	mvcur(FIRSTROW-1,field1.fc);
	for (i = 0; i < nbytes; printf(pmask[MASK(1)],(i++ + hnum)&radix));
	mvcur(FIRSTROW-1,field2.fc);
	for (i = 0; i < nbytes; printf("%1x",(i++ + hnum)&radix));
}

/*
 *								printinfo(L)
 *	NAME
 *		printinfo -- Display file size and cursor position.
 *
 *	SYNOPSIS
 *		void printinfo()
 *
 *	DESCRIPTION
 *		It displays file name, status (modified or not), size and
 *		cursor position relative to beginning of file.
 *
 *	RETURN VALUE
 *		None.
 */

static void printinfo() {

	mvcur(statusline,1);
	cleareos();
	printf("\"%s\" ",inpfname);
	if (~nochange)
		printf(mesg[23]);
	printsize(curflength,1);
	if (maxcol < 60)
		printf("\n\rcursor at ");
	else
		printf("  --  at ");
	printsize(base+actpos, 0);
	mvcur(y,x);
}

/*
 *								printsize(L)
 *	NAME
 *		printsize -- Display a long integer.
 *
 *	SYNOPSIS
 *		void printsize(sz, mode)
 *		long sz;
 *		int mode;
 *
 *	DESCRIPTION
 *		It displays the long integer `sz' accordingly to current
 *		numeric base. If `mode' is not zero, in addition it displays
 *		the string " bytes".
 *
 *	RETURN VALUE
 *		None.
 */

static void printsize(sz,mode)

 long	sz;
 int	mode;

{
	if (radix == 7)
		if (sz < 8L)
			printf("%ld",sz);
		else
			printf("%lo (%ld dec)",sz,sz);
	else
		if (sz < 10L)
			printf("%ld",sz);
		else
			printf("%lx (%ld dec)",sz,sz);

	if (mode)
		printf(" bytes");
}

/*
 *								prpageno(L)
 *	NAME
 *		prpageno -- Display page #.
 *
 *	SYNOPSIS
 *		void prpageno()
 *
 *	DESCRIPTION
 *		It displays the # of current page.
 *
 *	RETURN VALUE
 *		None.
 */

static void prpageno() {

	mvcur(1,maxcol-20);
	pageno = (int) (base / pagesz + 1);
	if ((radix == 7 && pageno < 8) || (radix == 15 && pageno < 10))
		printf("page %d", pageno);
	else
		printf(pmask[MASK(6)], pageno, pageno);
}

/*
 *								putch(L)
 *	NAME
 *		putch -- Call putchar().
 *
 *	SYNOPSIS
 *		void putch(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It's used to interface some termcap primitives.
 *
 *	RETURN VALUE
 *		None.
 */

static void putch(c)

 char c;

{
	putchar(c);
}

/*
 *								readtty(L)
 *	NAME
 *		readtty -- Get an item from stdin.
 *
 *	SYNOPSIS
 *		int readtty(c, ismchar)
 *		char *c;
 *		int (*ismchar)();
 *
 *	DESCRIPTION
 *		It reads an item (ASCII character or octal/hexadecimal code)
 *		and echoes it in the active field and its corresponding
 *		representation in the other field. The parameter `c' is used
 *		to return the ASCII (or ASCII equivalent) code read;
 *		`ismchar' is a function to be used to detect a match character.
 *
 *	RETURN VALUE
 *		It returns MATCH if a match character was detected by
 *		`ismchar' function, BS if a backspace was typed in, NORM
 *		otherwise.
 */

static int readtty(c,ismchar)

 register char	*c;
 int	(*ismchar)();

{
 int	i;
 int	ic;
 char	string[4];

	if (actfield == FIELD2) {

		while ((ic = GETACHAR) == EOF);

		*c = ic;
		if ((*ismchar)(*c))
			return(MATCH);

		if (*c == '\b')
			return(BS);

		if (*c == '\\') {
			while ((ic = GETACHAR) == EOF);
			*c = ic;
		}

		if (ISCTRL(*c))
			putchar('.');
		else
			putchar(*c);

		mvcur(y,(x-field2.fc)*field1.step+field1.fc);
		printf(pmask[MASK(2)],*c&255);
	}
	else {
		for (i = 0; i <= (15-radix)/5+1; i++) {
			if ((*ismchar)(*c = GETACHAR) && i == 0)
				return(MATCH);

			if (radix == 7 && i == 0)
				switch(*c) {
				case '0':
				case '1':
				case '2':
					string[i] = *c;
					putchar(*c);
					continue;

				default:
					putchar(BELL);
					i--;
					continue;
				}

			if ((*digitok)(*c)) {
				string[i] = *c;
				putchar(*c);
			}
			else
				if (*c == '\b')
					if (i > 0) {
						putchar('\b');
						i -= 2;
						continue;
					}
					else
						return(BS);
				else {
					putchar(BELL);
					i--;
				}
		}

		string[i] = '\0';

		/*
		** now display the ascii value
		*/

		sscanf(string,pmask[MASK(0)],&i);

		*c = (char) i;

		mvcur(y,(x-field1.fc)/field1.step+field2.fc);

		if (ISCTRL(*c))
			putchar('.');
		else
			putchar(*c);
	}
	return(NORM);
}

/*
 *								redraw(L)
 *	NAME
 *		redraw -- Redraw screen.
 *
 *	SYNOPSIS
 *		void redraw()
 *
 *	DESCRIPTION
 *		It redraws the screen.
 *
 *	RETURN VALUE
 *		None.
 */

static void redraw() {

	scrrefresh(base);
	mvcur(y,x);
}

/*
 *								resterm(L)
 *	NAME
 *		resterm -- Reset terminal.
 *
 *	SYNOPSIS
 *		void resterm()
 *
 *	DESCRIPTION
 *		It sets terminal to the state which the editor found it.
 *
 *	RETURN VALUE
 *		None.
 */

static void resterm() {

/* This function as originally coded using Version 7 screen handling is tjp */
/* now replaced by the System V screen handling                         tjp */
/*                                                                      tjp */
/*                                                                      tjp */
/*      ttyb.sg_flags = flag;                                           tjp */
/*      ioctl(2, TIOCSETP, &ttyb);                                      tjp */

        ioctl(0,TCSETS,&ttybsave);                                  /* tjp */
}

/*
 *								scrolldown(L)
 *	NAME
 *		scrolldown -- Scroll down.
 *
 *	SYNOPSIS
 *		void scrolldown()
 *
 *	DESCRIPTION
 *		It scrolls down half a page. If the cursor is at end of file,
 *		it generates a bell signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void scrolldown() {

 long	addr;
 int	dist;

	dist = CUROFFS(curfield.lr,curfield.lconlr);
	if (base + dist + 1  ==  curflength)
		if (actpos < dist) {
			logeop();
			return;
		}
		else {
			putchar(BELL);
			return;
		}
	addr = actpos + pagesz/2;
	if (base + addr >= curflength)
		addr = curflength - (base + actpos + 1);
	else
		addr -= addr%nbytes;

	base += addr - addr%nbytes;

	scrrefresh(base);
	if (CUROUTSIDE)
		logeop();
	else
		mvcur(y,x);
}

/*
 *								scrollup(L)
 *	NAME
 *		scrollup -- Scroll up.
 *
 *	SYNOPSIS
 *		void scrollup()
 *
 *	DESCRIPTION
 *		It scrolls up half a page. If the cursor is at the beginning
 *		of file, it generates a bell signal.
 *
 *	RETURN VALUE
 *		None.
 */

static void scrollup() {

	if (base > 0L) {
		base -= pagesz/2 + (pagesz/2)%nbytes;
		if (base < 0L)
			base = 0L;

		scrrefresh(base);
		mvcur(y,x);
	}
	else
		if (actpos > 0)
			loghome();
		else
			putchar(BELL);
}

/*
 *								scrrefresh(L)
 *	NAME
 *		scrrefresh -- Display an entire page.
 *
 *	SYNOPSIS
 *		int scrrefresh(address)
 *		long address;
 *
 *	DESCRIPTION
 *		It displays the page with address `address', redrawing
 *		all the screen. If `address' is out of file address space,
 *		it does nothing.
 *
 *	RETURN VALUE
 *		If `address' is less than file size, it returns the length
 *		of the page displayed, otherwise -1.
 */

static int scrrefresh(address)

 long	address;

{

	if (curflength > 0L)
		if (address < curflength) {

			hnum = address%(radix+1);
			printhdr();

			lseek(tmpfd,address,FSEEK_ABSOLUTE);
			l = read(tmpfd,rbuffer,min(pagesz,curflength-address));
			display(FIRSTROW,address);
			return(l);
		}
		else
			return(-1);
	else {
		hnum = 0;
		printhdr();
		l = 0;
		display(FIRSTROW,0L);
		return(l);
	}
}

/*
 *								search(L)
 *	NAME
 *		search -- Search for a pattern.
 *
 *	SYNOPSIS
 *		long search(fptr, patt, pl, dir)
 *		long fptr;
 *		char *patt;
 *		int pl, dir;
 *
 *	DESCRIPTION
 *		It searches for pattern `patt' of length `pl' in the file with
 *		descriptor `fptr' in the direction `dir'.
 *
 *	RETURN VALUE
 *		If it found the pattern, it returns its address, otherwise
 *		-1.
 */

static long search(fptr,patt,pl,dir)

 long	fptr;
 register char	*patt;
 int	pl,dir;

{
 int	i;
 int	ref;

	if (dir == FORW) {
		fptr++;
		ref = BLKSIZE - pl;
		do {
			lseek(tmpfd,fptr,FSEEK_ABSOLUTE);
			l = min(read(tmpfd,rbuffer,BLKSIZE),curflength-fptr);
			for (i = 0; i <= l-pl; i++)
				if (strncmp(patt,&rbuffer[i],pl) == 0)
					return(fptr+i);
			fptr += ref;
		} while (l == BLKSIZE);
	}
	else
		do {
			ref = min(BLKSIZE,fptr);
			fptr -= ref;
			lseek(tmpfd,fptr,FSEEK_ABSOLUTE);
			l = read(tmpfd,rbuffer,ref);
			for (i = l-pl; i >= 0; i--)
				if (strncmp(patt,&rbuffer[i],pl) == 0)
					return(fptr+i);
		} while (fptr > 0L);

	return(-1L);
}

/*
 *								b_select(L)
 *	NAME
 *		select -- Change numeric base.
 *
 *	SYNOPSIS
 *		void b_select(reprtype)
 *		int reprtype;
 *
 *	DESCRIPTION
 *		Accordingly with `reprtype' choosen (OCT or HEX), it
 *		computes new values for the variables used to manage
 *		page displaying.
 *
 *	RETURN VALUE
 *		None.
 */

static void b_select(reprtype)

 int	reprtype;

{

	nbytes = (maxcol - reprtype) / (reprtype/3);

	if (reprtype == OCT) {
		if (nbytes > 8)
			nbytes -= nbytes % 8;	/* adjust bytes/row to a */
						/* multiple of 8 */
		digitok = isoct;
		radix = 7;
		field1.fc = ob;
		field1.lcmax = ob + (nbytes - 1)*4;
		field1.step = OCTSTEP;
		field2.fc = field1.lcmax + 6;
	}
	else {
		if (nbytes > 16)
			nbytes -= nbytes % 16;	/* adjust bytes/row to a */
						/* multiple of 16 */
		digitok = ishex;
		radix = 15;
		field1.fc = xb;
		field1.lcmax = xb + (nbytes - 1)*3;
		field1.step = HEXSTEP;
		field2.fc = field1.lcmax + 5;
	}

	field2.lcmax = field2.fc + nbytes - 1;
	pagesz = nbytes * nrows;
}

#ifdef PC
static setparam() {

	mvcur(1,1);
	cleareos();
	for (i = 0; i < sizeof(option)/sizeof(struct opt_s); i++) {

		switch (option[i].type) {
		case RANGE:
			printf("%s = %d", option[i].longname, option[i].curvalue);
			mvcur(i+1, 21);
			printf("(%s, %s)\n\r", option[i].value[0], option[i].value[1]);
			break;

		case LIST:
			printf("%s = %d", option[i].longname, option[i].curvalue);
			mvcur(i+1, 21);
			printf("(%s", option[i].value[0]);
			for (j = 1; j < option[i].nvalues; j++)
				printf(", %s", option[i].value[j]);
			printf(")\n\r");
			break;

		case BOOL:
			if (option[i].curvalue == FALSE)
				printf("no");
			printf("%-20s(y, n)\n\r", option[i].longname);
			break;
		}
	}

	for (i = 0; i < sizeof(option)/sizeof(struct opt_s); i++) {
		mvcur(i+1,option[i].optlength+4);
		for (j = 0; (rbuffer[j] = GETACHAR) != '\n'; j++)
			putchar(rbuffer[j]);
		rbuffer[j] = '\0';

		switch (option[i].type) {

		case RANGE:
			for (;;)
				if (strcmp(rbuffer, option[i].value[0]) >= 0 &&
			    	strcmp(rbuffer, option[i].value[1]) <= 0) {
					option[i].curvalue = atoi(rbuffer);
					break;
				}
				else {
					putchar(BELL);
					mvcur(i+1,option[i].optlength+4);
				}
			break;
		/* t.b.c. */
		}
	}
}
#endif

/*
 *								setterm(L)
 *	NAME
 *		setterm -- Set terminal.
 *
 *	SYNOPSIS
 *		int setterm()
 *
 *	DESCRIPTION
 *		It sets terminal to operate without automatic echo and
 *		tabulation characters expansion and with the availability
 *		of single characters as they are typed in.
 *
 *	RETURN VALUE
 *		It returns 0 if the call was successful, otherwise -1.
 */

static int setterm() {

/* This function as originally coded using Version 7 screen handling is tjp */
/* now replaced by the System V screen handling                         tjp */
/*
	if (ioctl(2, TIOCGETP, &ttyb) == -1)
		return(-1);
	flag = ttyb.sg_flags;

#ifdef SIGTSTP
	signal(SIGTSTP,stopproc);
#endif

	if (signal(SIGINT,SIG_IGN) != SIG_IGN)
		signal(SIGINT,intrproc);

	ttyb.sg_flags = ttyb.sg_flags & ~ECHO & ~XTABS |

#ifdef CBREAK
				CBREAK;
#else
			CRMOD |  NL1 | CR1 | RAW;
#endif

	if (ioctl(2, TIOCSETP, &ttyb) == -1)
		return(-1);
	else
		return(0);
                                                                   /* tjp */

	if (ioctl(0, TCGETS, &ttyb) == -1)                         /* tjp */
		return(-1);                                        /* tjp */
        ttybsave=ttyb;                                             /* tjp */

#ifdef SIGTSTP                                                     /* tjp */
	signal(SIGTSTP,stopproc);                                  /* tjp */
#endif                                                             /* tjp */

	if (signal(SIGINT,SIG_IGN) != SIG_IGN)                     /* tjp */
		signal(SIGINT,intrproc);                           /* tjp */

        ttyb.c_iflag &= ~(INLCR | ICRNL | IUCLC | ISTRIP
                               | BRKINT | IGNPAR);                 /* tjp */
        ttyb.c_oflag &= ~(OPOST);                                  /* tjp */
        ttyb.c_lflag &= ~(ICANON | ISIG | ECHO);                   /* tjp */
#ifdef I_DONT_UNDERSTAND_WHY_YOU_WOULD_WANT_TO_DO_THIS
        ttyb.c_cc[4] = 5; /* MIN  */                               /* tjp */
        ttyb.c_cc[5] = 2; /* TIME */                               /* tjp */
#else
        ttyb.c_cc[4] = 1; /* MIN  */                               /* pds */
        ttyb.c_cc[5] = 0; /* TIME */                               /* pds */
#endif

        if (ioctl (0,TCSETS,&ttyb) == -1)                         /* tjp */
          return(-1);                                              /* tjp */
	else                                                       /* tjp */
          return(0);                                               /* tjp */
}

/*
 *								spec_match(L)
 *	NAME
 *		spec_match -- Check whether a character is a match character.
 *
 *	SYNOPSIS
 *		int spec_match(c)
 *		char c;
 *
 *	DESCRIPTION
 *		It checks whether `c' is ESC, LF or TAB.
 *
 *	RETURN VALUE
 *		Boolean value.
 */

static int spec_match(c)

 char	c;

{
	return(c == ESC || c == '\n' || c == '\t');
}

/*
 *								sreplace(L)
 *	NAME
 *		sreplace -- Replace a single byte.
 *
 *	SYNOPSIS
 *		void sreplace()
 *
 *	DESCRIPTION
 *		It replaces a single byte.
 *
 *	RETURN VALUE
 *		None.
 */

static void sreplace() {

 char	c;

	while (readtty(&c,nevermatch) != NORM)
		putchar(BELL);

	mvcur(y,x);

	lseek(tmpfd,base+actpos,FSEEK_ABSOLUTE);
	nochange = FALSE;
	if (write(tmpfd,&c,1) != 1)
		error(WRITEERR);
}

/*
 *								startup(L)
 *	NAME
 *		startup -- Initialize environment.
 *
 *	SYNOPSIS
 *		int startup(count, par)
 *		int count;
 *		char **par;
 *
 *	DESCRIPTION
 *		It opens the file to be edited, creates two working files,
 *		initializes some variables, sets the default representation
 *		to hexadecimal, sets some terminal flags and displays the first
 *		page of the file.
 *
 *	RETURN VALUE
 *		It returns STOP if something failed, OK otherwise.
 */

static int startup (count,par)

 int	count;
 register char	**par;

{
 int	pid;
 int	repr;

#ifdef SIGTSTP
	signal(SIGTSTP,stopignore);
#endif
	if (count == 1) {
		printf (mesg[24],par[0]);
		return (STOP);
	}
	if (gettcap() == NOTOK) {
		printf (mesg[25],par[0]);
		return(STOP);
	}

	if (par[1][0] == '-')
		if (par[1][1] == 'o') {
			strcpy(inpfname,par[2]);
			repr = OCT;
		}
		else {
			printf (mesg[26],par[0],par[1]);
			return (STOP);
		}
	else {
		strcpy(inpfname,par[1]);
		repr = HEX;
	}

	nrows = maxrow - (FIRSTROW - 1) - 2;
	b_select(repr);

	field2.step = ASCSTEP;

	actfield = FIELD1;

	y = FIRSTROW;
	x = curfield.fc = field1.fc;
	curfield.lcmax = field1.lcmax;
	curfield.step = field1.step;


	pinfo = &info;

	if (maxcol < 80)
		statusline = maxrow - 1;
	else
		statusline = maxrow;

	SETLASTROW;

	if (stat(inpfname,pinfo) == -1) {
		if ((inpfd = creat(inpfname,0644)) == -1) {
			printf (mesg[27],par[0],inpfname);
			return(STOP);
		}
		newfile = TRUE;
		close(inpfd);
		if ((inpfd = open (inpfname,FATT_RDWR)) == -1) {
			printf (mesg[28],par[0],inpfname);
			return (STOP);
		}
	}
	else {
		newfile = FALSE;
		curflength = pinfo->st_size;
	}

	mvcur(1,1);
	cleareos();
	mvcur(statusline,1);
	printf("\"%s\" ",inpfname);
	fflush(stdout);

	if (!newfile)
		if ((inpfd = open (inpfname,FATT_RDWR)) == -1)
			if ((inpfd = open (inpfname,FATT_RDONLY)) == -1) {
				printf(mesg[38]);
				return (STOP);
			}

	if (setterm() == -1)
		return(STOP);

	base = 0L;

	pid = getpid();
	sprintf(tmpfname,"/tmp/Xvi%d",pid);
	sprintf(auxfname,"/tmp/XVI%d",pid);

	if ((tmpfd = creat(tmpfname,0644)) == -1) {
		mvcur(statusline,1);
		cleareos();
		printf (mesg[29],par[0]);
		return (STOP);
	}

	if (~newfile)
		copyblk(inpfd,tmpfd,0L,curflength-1,0L);

	close(inpfd);
	close(tmpfd);
	if ((tmpfd = open(tmpfname,FATT_RDWR)) == -1) {
		mvcur(statusline,1);
		cleareos();
		printf(mesg[30],par[0]);
		return(STOP);
	}

	if ((auxfd = creat(auxfname,0644)) == -1) {
		mvcur(statusline,1);
		cleareos();
		printf (mesg[31],par[0]);
		return (STOP);
	}

	close(auxfd);

	if ((auxfd = open (auxfname,FATT_RDWR)) == -1) {
		mvcur(statusline,1);
		cleareos();
		printf (mesg[32],par[0]);
		return (STOP);
	}

	/* read the first page */

	l = read (tmpfd,rbuffer,pagesz);

	if (~newfile)
		printsize(curflength,1);
	else
		printf(mesg[33]);

	fflush(stdout);
	sleep(2);

	printhdr();
	firstdisplay = TRUE;
	display(FIRSTROW,0L);
	firstdisplay = FALSE;
	mvcur(statusline,1);

	printf("\"%s\" ",inpfname);
	if (newfile)
		printf(mesg[33]);
	else
		printsize(curflength,1);
	printf("       (type <control-?> for help)");
	mvcur(y,x);

	return (OK);
}

/*
 *								stoln(L)
 *	NAME
 *		stoln -- Convert a string into a long integer.
 *
 *	SYNOPSIS
 *		int stoln(string, num, eosdet)
 *		char *string;
 *		long *num;
 *		int (*eosdet)();
 *
 *	DESCRIPTION
 *		It converts the content of `string', bounded by position
 *		determined by function `eosdet', into the long integer
 *		pointed by `num'. The input `string' is assumed to be
 *		- a hex number, if it's like `0xn';
 *		- a oct number, if it's like `0n';
 *		- a decimal number, if it's like `n' (n != 0).
 *		No sign is expected.
 *
 *	RETURN VALUE
 *		It returns 0 if the conversion was possible, -1 otherwise.
 */

static int stoln(string,num,eosdet)

 register char	*string;
 long	*num;
 int	(*eosdet)();

{
 char	csave;
 int	i;
 int	k;
 int	(* digok)();
 char	*sp;
 char   *sp2;

	if (string[1] == 'x') {
		digok = ishex;
		string += 2;
		k = 13;
	}
	else
		if (string[0] == '0') {
			digok = isoct;
			k = 5;
		}
		else {
			digok = myisdigit;
			k = 16;
		}

	sp = string;
	sp2 = string;
	while ((*digok)(*string++));
	while (!(*eosdet)(*sp2++));
	if (string != sp2)
		return(-1);
	csave = *--string;

	*string == '\0';
	if (sscanf(sp,pmask[k],num) != -1)
		k = 0;
	else
		k = -1;

	*string = csave;
	return(k);
}

/*
 *								stopignore(L)
 *	NAME
 *		stopignore --  A SIGTSTP interrupt service routine.
 *
 *	SYNOPSIS
 *		void stopignore()
 *
 *	DESCRIPTION
 *		This function is called when a SIGTSTP (ctrl-z) interrupt
 *		signal is generated. It does nothing.
 *
 *	RETURN VALUE
 *		None.
 */

#ifdef SIGTSTP
static void stopignore() {

	signal(SIGTSTP,stopignore);
}
#endif

/*
 *								stopproc(L)
 *	NAME
 *		stopproc --  A SIGTSTP interrupt service routine.
 *
 *	SYNOPSIS
 *		void stopproc()
 *
 *	DESCRIPTION
 *		This function is called when a SIGTSTP signal is generated.
 *		It outputs a bell tone (used during updates).
 *
 *	RETURN VALUE
 *		None.
 */

#ifdef SIGTSTP
static void stopproc() {

	signal(SIGTSTP,stopproc);
	putchar(BELL);
	fflush(stdout);
}
#endif

/*
 *								terminate(L)
 *	NAME
 *		terminate -- Terminate an editing session.
 *
 *	SYNOPSIS
 *		void terminate()
 *
 *	DESCRIPTION
 *		It closes and removes working files; if input file didn't
 *		exist and is empty, it's removed.
 *
 *	RETURN VALUE
 *		None.
 */

static void terminate() {

	quit = TRUE;
	if (tmpfd != -1) {
		close(tmpfd);
		unlink(tmpfname);
	}
	if (auxfd != -1) {
		close(auxfd);
		unlink(auxfname);
	}
	if (inpfd != -1) {
		close(inpfd);
		if (newfile && curflength == 0L)
			unlink(inpfname);
	}
	printf("\n\r");
}

/*
 *								toeosrefresh(L)
 *	NAME
 *		toeosrefresh -- Draw a portion of screen.
 *
 *	SYNOPSIS
 *		void toeosrefresh(address)
 *		long address;
 *
 *	DESCRIPTION
 *		It displays a portion of file from current screen line
 *		to end of screen; the portion of file begins at `address',
 *		which must be coherent with that of the preceding line.
 *
 *	RETURN VALUE
 *		None.
 */

static void toeosrefresh(address)

 long	address; /* it MUST be the address of the beginning of a line */

{

	lseek(tmpfd,address,FSEEK_ABSOLUTE);
	l = read(tmpfd,rbuffer,pagesz-(actpos- actpos%nbytes));
	if (address + l > curflength)
		l = curflength - address;
	if (pageno != base/pagesz)
		prpageno();
	display(y,address);
}

/*
 *								wrnewf(L)
 *	NAME
 *		wrnewf -- Save a portion of working file into a new one.
 *
 *	SYNOPSIS
 *		int wrnewf(cmd, cmdlength, from, to)
 *		char *cmd;
 *		int cmdlength;
 *		long from, to;
 *
 *	DESCRIPTION
 *		It checks whether `cmd' contains a file name, ranging from 0
 *		to `cmdlength': if a name is found, the working file from
 *		`from' to `to' is saved into the specified file, destroying
 *		its previous content if a `!' is present in the command.
 *
 *	RETURN VALUE
 *		It returns:
 *			OK	if saving was successful;
 *			NOFNAME if file name is missing;
 *			NOTOK	if file name is not legal or it can't be
 *				created/opened or it already exists and the
 *				`!' wasn't specified in the command.
 */

static int wrnewf(cmd,cmdlength,from,to)

 register char	*cmd;
 int	cmdlength;
 long	from;
 long	to;

{
 int	j,iaux,created;


	for (j = 2; j < cmdlength && isspace(cmd[j]); j++);
	if (j < cmdlength) {

		/*
		** maybe a filename
		*/

		cleareos();
		printf("\"%s\" ",&cmd[j]);
		fflush(stdout);
		if ((! isspace(cmd[1]) && cmd[1] != '!') || ! isvalidname(&cmd[j])) {
			printf(mesg[34]);
			return(NOTOK);
		}
		if (stat(&cmd[j],pinfo) == -1) {
			if ((iaux = creat(&cmd[j],0644)) == -1) {
				printf(mesg[5]);
				return(NOTOK);
			}
			close(iaux);
			created = TRUE;
		}
		else
			created = FALSE;
		if ((iaux = open(&cmd[j],FATT_WRONLY)) == -1) {
			printf(mesg[5]);
			return(NOTOK);
		}
		if (! created && cmd[1] != '!') {
			printf(mesg[35],&cmd[j]);
			close(iaux);
			return(NOTOK);
		}
		copyblk(tmpfd,iaux,from,to,0L);
		to -= from - 1;
		printf(mesg[33]);
		printsize(to,1);
		return(OK);
	}
	else
		return(NOFNAME);
}

/*
 *								wrsamef(L)
 *	NAME
 *		wrsamef -- Save a portion of working file into input file.
 *
 *	SYNOPSIS
 *		int wrsamef(cmd, addr1, addr2)
 *		char *cmd;
 *		long addr1, addr2;
 *
 *	DESCRIPTION
 *		It saves the portion of working file between `addr1' and
 *		`addr2' into the input file. If the command `cmd' contains
 *		`x' or `wq', it terminates editing session. In any case, it
 *		closes input file.
 *
 *	RETURN VALUE
 *		It returns:
 *			OK	if saving was successful;
 *			NOTOK	if input file can't be rewritten;
 *			QUIT	if saving was successful and quit was issued.
 */

static int wrsamef(cmd,addr1,addr2)

 register char	*cmd;
 long	addr1,addr2;

{

	cleareos();
	printf("\"%s\" ",inpfname);
	fflush(stdout);
	if ((inpfd = creat(inpfname,0644)) == -1) {
		printf(mesg[5]);
		return(NOTOK);
	}
	close(inpfd);
	if ((inpfd = open (inpfname,FATT_WRONLY)) == -1) {
		printf(mesg[5]);
		return(NOTOK);
	}
	copyblk(tmpfd,inpfd,addr1,addr2,0L);
	addr2 -= addr1 - 1;
	printsize(addr2,1);
	if (cmd[0] == 'x' || cmd[1] == 'q') {
		terminate();
		return(QUIT);
	}
	else {
		close(inpfd);
		return(OK);
	}
}

