/* input.c: i/o routines for files and pseudo-files (strings) */

#include <errno.h>
#include <setjmp.h>
#include "rc.h"
#include "jbwrap.h"

/*
   NB: character unget is supported for up to two characters, but NOT
   in the case of EOF. Since EOF does not fit in a char, it is easiest
   to support only one unget of EOF.
*/

typedef struct Input {
	inputtype t;
	char *ibuf;
	int fd, index, read, lineno, last;
	bool saved, eofread;
} Input;

#define BUFSIZE ((SIZE_T) 256)

#ifdef READLINE
extern char *readline(char *);
extern void add_history(char *);
static char *rlinebuf;
#endif

char *prompt, *prompt2;
bool rcrc;

static int dead(void);
static int fdgchar(void);
static int stringgchar(void);
static void history(void);
static void ugdead(int);
static void pushcommon(void);

static char *inbuf;
static SIZE_T istacksize, chars_out, chars_in;
static bool eofread = FALSE, save_lineno = TRUE;
static Input *istack, *itop;

static int (*realgchar)(void);
static void (*realugchar)(int);

int last;

extern int gchar() {
	int c;

	if (eofread) {
		eofread = FALSE;
		return last = EOF;
	}

	while ((c = (*realgchar)()) == '\0')
		pr_error("warning: null character ignored");

	return c;
}

extern void ugchar(int c) {
	(*realugchar)(c);
}

static int dead() {
	return last = EOF;
}

static void ugdead(int c) {
	return;
}

static void ugalive(int c) {
	if (c == EOF)
		eofread = TRUE;
	else
		inbuf[--chars_out] = c;
}

/* get the next character from a string. */

static int stringgchar() {
	return last = (inbuf[chars_out] == '\0' ? EOF : inbuf[chars_out++]);
}

/* signal-safe readline wrapper */

#ifdef READLINE
#ifndef SVSIGS
static char *rc_readline(char *prompt) {
	char *r;
	interrupt_happened = FALSE;
	if (!setjmp(slowbuf.j)) {
		slow = TRUE;
		if (!interrupt_happened)
			r = readline(prompt);
		else
			r = NULL;
	} else
		r = NULL;
	slow = FALSE;
	if (r == NULL)
		errno = EINTR;
	sigchk();
	return r;
}
#else
#define rc_readline readline
#endif /* SVSIGS */
#endif /* READLINE */

/*
   read a character from a file-descriptor. If GNU readline is defined, add a newline and doctor
   the buffer to look like a regular fdgchar buffer.
*/

static int fdgchar() {
	if (chars_out >= chars_in + 2) { /* has the buffer been exhausted? if so, replenish it */
		while (1) {
#ifdef READLINE
			if (interactive && istack->fd == 0) {
				rlinebuf = readline(prompt);
				if (rlinebuf == NULL) {
					chars_in = 0;
				} else {
					if (*rlinebuf != '\0')
						add_history(rlinebuf);
					chars_in = strlen(rlinebuf) + 1;
					efree(inbuf);
					inbuf = ealloc(chars_in + 3);
					strcpy(inbuf+2, rlinebuf);
					strcat(inbuf+2, "\n");
					efree(rlinebuf);
				}
			} else
#endif
				{
				long /*ssize_t*/ r = rc_read(istack->fd, inbuf + 2, BUFSIZE);
				sigchk();
				if (r < 0) {
					uerror("read");
					rc_exit(1);
				}
				chars_in = (SIZE_T) r;
			}
			break;
		}
		if (chars_in == 0)
			return last = EOF;
		chars_out = 2;
		if (dashvee)
			writeall(2, inbuf + 2, chars_in);
		history();
	}
	return last = inbuf[chars_out++];
}

/* set up the input stack, and put a "dead" input at the bottom, so that yyparse will always read eof */

extern void initinput() {
	istack = itop = ealloc(istacksize = 256 * sizeof (Input));
	istack->t = iFd;
	istack->fd = -1;
	realugchar = ugalive;
}

/* push an input source onto the stack. set up a new input buffer, and set gchar() */

static void pushcommon() {
	SIZE_T idiff;
	istack->index = chars_out;
	istack->read = chars_in;
	istack->ibuf = inbuf;
	istack->lineno = lineno;
	istack->saved = save_lineno;
	istack->last = last;
	istack->eofread = eofread;
	istack++;
	idiff = istack - itop;
	if (idiff >= istacksize / sizeof (Input)) {
		itop = erealloc(itop, istacksize *= 2);
		istack = itop + idiff;
	}
	realugchar = ugalive;
	chars_out = 2;
	chars_in = 0;
}

extern void pushfd(int fd) {
	pushcommon();
	istack->t = iFd;
	save_lineno = TRUE;
	istack->fd = fd;
	realgchar = fdgchar;
	inbuf = ealloc(BUFSIZE + 2);
	lineno = 1;
}

extern void pushstring(char **a, bool save) {
	pushcommon();
	istack->t = iString;
	save_lineno = save;
	inbuf = mprint("..%A", a);
	realgchar = stringgchar;
	if (save_lineno)
		lineno = 1;
	else
		--lineno;
}

/* remove an input source from the stack. restore the right kind of getchar (string,fd) etc. */

extern void popinput() {
	if (istack->t == iFd)
		close(istack->fd);
	efree(inbuf);
	--istack;
	realgchar = (istack->t == iString ? stringgchar : fdgchar);
	if (istack->t == iFd && istack->fd == -1) { /* top of input stack */
		realgchar = dead;
		realugchar = ugdead;
	}
	last = istack->last;
	eofread = istack->eofread;
	inbuf = istack->ibuf;
	chars_out = istack->index;
	chars_in = istack->read;
	if (save_lineno)
		lineno = istack->lineno;
	else
		lineno++;
	save_lineno = istack->saved;
}

/* flush input characters upto newline. Used by scanerror() */

extern void flushu() {
	int c;
	if (last == '\n' || last == EOF)
		return;
	while ((c = gchar()) != '\n' && c != EOF)
		; /* skip to newline */
	if (c == EOF)
		ugchar(c);
}

/* the wrapper loop in rc: prompt for commands until EOF, calling yyparse and walk() */

extern Node *doit(bool execit) {
	bool eof;
	Jbwrap j;
	Estack e1, e2;
	Edata jerror;
	if (dashen)
		execit = FALSE;
	setjmp(j.j);
	jerror.jb = &j;
	except(eError, jerror, &e1);
	for (eof = FALSE; !eof;) {
		Edata block;
		block.b = newblock();
		except(eArena, block, &e2);
		sigchk();
		if (dashell) {
			char *fname[3];
			fname[1] = concat(varlookup("home"), word("/.rcrc", NULL))->w;
			fname[2] = NULL;
			rcrc = TRUE;
			dashell = FALSE;
			b_dot(fname);
		}
		if (interactive) {
			List *s;
			if (!dashen && fnlookup("prompt") != NULL) {
				static char *arglist[] = { "prompt", NULL };
				funcall(arglist);
			}
			if ((s = varlookup("prompt")) != NULL) {
#ifdef READLINE
				prompt = s->w;
#else
				fprint(2, "%s", s->w);
#endif
				prompt2 = (s->n == NULL ? "" : s->n->w);
			}
		}
		inityy();
		if (yyparse() == 1 && execit)
			rc_raise(eError);
		eof = (last == EOF); /* "last" can be clobbered during a walk() */
		if (parsetree != NULL) {
			if (execit)
				walk(parsetree, TRUE);
			else if (dashex && dashen)
				fprint(2, "%T\n", parsetree);
		}
		unexcept(); /* eArena */
	}
	popinput();
	unexcept(); /* eError */
	return parsetree;
}

/* parse a function imported from the environment */

extern Node *parseline(char *extdef) {
	int i = interactive;
	char *in[2];
	Node *fun;
	in[0] = extdef;
	in[1] = NULL;
	interactive = FALSE;
	pushstring(in, TRUE);
	fun = doit(FALSE);
	interactive = i;
	return fun;
}

/* write last command out to a file if interactive && $history is set */

static void history() {
	List *hist;
	SIZE_T a;

	if (!interactive || (hist = varlookup("history")) == NULL)
		return;

	for (a = 0; a < chars_in; a++) {
		char c = inbuf[a+2];

		/* skip empty lines and comments */
		if (c == '#' || c == '\n')
			break;

		/* line matches [ \t]*[^#\n] so it's ok to write out */
		if (c != ' ' && c != '\t') {
			char *name = hist->w;
			int fd = rc_open(name, rAppend);
			if (fd < 0) {
				uerror(name);
				varrm(name, TRUE);
			} else {
				writeall(fd, inbuf + 2, chars_in);
				close(fd);
			}
			break;
		}
	}
}

/* close file descriptors after a fork() */

extern void closefds() {
	Input *i;
	for (i = istack; i != itop; --i)	/* close open scripts */
		if (i->t == iFd && i->fd > 2) {
			close(i->fd);
			i->fd = -1;
		}
}
