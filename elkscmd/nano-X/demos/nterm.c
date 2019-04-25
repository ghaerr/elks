#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "nano-X.h"
/*
 * Nano-X terminal emulator
 *
 * Al Riddoch
 * Greg Haerr
 */
#define HAVEBLIT 0		/* set if have bitblit (experimental)*/

#define	_	((unsigned) 0)		/* off bits */
#define	X	((unsigned) 1)		/* on bits */
#define	MASK(a,b,c,d,e,f,g) \
	(((((((((((((a * 2) + b) * 2) + c) * 2) + d) * 2) \
	+ e) * 2) + f) * 2) + g) << 9)

static GR_WINDOW_ID	w1;	/* id for window */
static GR_GC_ID		gc1;	/* graphics context */
static GR_GC_ID		gc3;	/* graphics context */
static GR_COORD		xpos;	/* x coord for text */
static GR_COORD		ypos;	/* y coord for text */
static GR_SCREEN_INFO	si;	/* screen info */
static int 		tfd;

void do_buttondown();
void do_buttonup();
void do_motion();
void text_init();
int term_init();
void do_keystroke();
void do_focusin();
void do_focusout();
void do_enter();
void do_exit();
void do_fdinput();
void errorcatcher();			/* routine to handle errors */
void printg();

int main(int argc, char ** argv)
{
	GR_EVENT	event;
	GR_BITMAP	bitmap1fg[7];	/* mouse cursor */
	GR_BITMAP	bitmap1bg[7];

	if (GrOpen() < 0) {
		fprintf(stderr, "cannot open graphics\n");
		exit(1);
	}
	
	GrGetScreenInfo(&si);

	GrSetErrorHandler(errorcatcher);

	w1 = GrNewWindow(GR_ROOT_WINDOW_ID, 50, 30, si.cols - 80,
		si.rows - 60, 1, WHITE, LTBLUE);

	GrSelectEvents(w1, GR_EVENT_MASK_BUTTON_DOWN |
		GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_EXPOSURE |
		GR_EVENT_MASK_FOCUS_IN | GR_EVENT_MASK_FOCUS_OUT);

	GrMapWindow(w1);

	gc1 = GrNewGC();
	gc3 = GrNewGC();

	GrSetGCForeground(gc1, GRAY);
	GrSetGCBackground(gc1, LTBLUE);
	GrSetGCFont(gc1, FONT_OEM_FIXED);
	/*GrSetGCFont(gc1, FONT_GUI_VAR);*/
	GrSetGCForeground(gc3, WHITE);
	GrSetGCBackground(gc3, BLACK);

	bitmap1fg[0] = MASK(_,_,X,_,X,_,_);
	bitmap1fg[1] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[2] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[3] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[4] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[5] = MASK(_,_,_,X,_,_,_);
	bitmap1fg[6] = MASK(_,_,X,_,X,_,_);

	bitmap1bg[0] = MASK(_,X,X,X,X,X,_);
	bitmap1bg[1] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[2] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[3] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[4] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[5] = MASK(_,_,X,X,X,_,_);
	bitmap1bg[6] = MASK(_,X,X,X,X,X,_);

	GrSetCursor(w1, 7, 7, 3, 3, WHITE, BLACK, bitmap1fg, bitmap1bg);

	/*GrFillRect(GR_ROOT_WINDOW_ID, gc1, 0, 0, si.cols, si.rows);*/

	GrSetGCForeground(gc1, BLACK);
	GrSetGCBackground(gc1, WHITE);
	text_init();
	if (term_init() < 0) {
		GrClose();
		exit(1);
	}

	/* we want tfd events also*/
	GrRegisterInput(tfd);

	while (1) {
		GrGetNextEvent(&event);

		switch (event.type) {
			case GR_EVENT_TYPE_KEY_DOWN:
				do_keystroke(&event.keystroke);
				break;

			case GR_EVENT_TYPE_FOCUS_IN:
				do_focusin(&event.general);
				break;

			case GR_EVENT_TYPE_FOCUS_OUT:
				do_focusout(&event.general);
				break;

			case GR_EVENT_TYPE_FDINPUT:
				do_fdinput();
				break;
		}
	}
}

#if ELKS
char * nargv[2] = {"/bin/sash", NULL};
#else
char * nargv[2] = {"/bin/sh", NULL};
#endif

void sigchild(int signo)
{
	printg("We have a signal right now!\n");
	GrClose();
	exit(0);
}

int term_init()
{
	char pty_name[12];
	int n = 0;
	pid_t pid;

again:
	sprintf(pty_name, "/dev/ptyp%d", n);
	if ((tfd = open(pty_name, O_RDWR | O_NONBLOCK)) < 0) {
		if ((errno == EBUSY) && (n < 3)) {
			n++;
			goto again;
		}
		fprintf(stderr, "Can't create pty %s\n", pty_name);
		return -1;
	}
	signal(SIGCHLD, sigchild);
	signal(SIGINT, sigchild);
	if ((pid = fork()) == -1) {
		fprintf(stderr, "No processes\n");
		return -1;
	}
	if (!pid) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		close(tfd);
		
		setsid();
		pty_name[5] = 't'; /* results in: /dev/ttyp%d */
		if ((tfd = open(pty_name, O_RDWR)) < 0) {
			fprintf(stderr, "Child: Can't open pty %s\n", pty_name);
			exit(1);
		}
		dup2(tfd, STDIN_FILENO);
		dup2(tfd, STDOUT_FILENO);
		dup2(tfd, STDERR_FILENO);
		execv(nargv[0], nargv);
		exit(1);
	}
	return 0;
}
	

GR_SIZE		width;		/* width of character */
GR_SIZE		height;		/* height of character */
GR_SIZE		base;		/* height of baseline */

void text_init()
{
	GrGetGCTextSize(gc1, "A", 1, &width, &height, &base);
}

void char_del(GR_COORD x, GR_COORD y)
{
	xpos -= width;
	GrFillRect(w1, gc3, x, y /*- height*/ /*+ base*/ + 1, width, height);
}
	
void char_out(GR_CHAR ch)
{
	switch(ch) {
	case '\r':
		xpos = 0;
		return;
	case '\n':
		xpos = 0;
		ypos += height;
		if(ypos > si.rows - 60 - height) {
			ypos -= height;
#if HAVEBLIT
			bogl_cfb8_blit(50, 30, si.cols-120,
				si.rows-60-height, 50, 30+height);
			GrFillRect(w1, gc3, 50, ypos, si.cols-120, height);
#else
			/* FIXME: changing FALSE to TRUE crashes nano-X*/
			/* clear screen, no scroll*/
			ypos = 0;
			GrClearWindow(w1, FALSE);
#endif
		}
		return;
	case '\007':			/* bel*/
		return;
	case '\t':
		xpos += width;
		while((xpos/width) & 7)
			char_out(' ');
		return;
	case '\b':			/* assumes fixed width font!!*/
		if (xpos <= 0)
			return;
		char_del(xpos, ypos);
		return;
	}
	GrText(w1, gc1, xpos, ypos + height /* + base */, &ch, 1);
	xpos += width;
}

void printg(char * text)
{
	int i;

	for(i = 0; i < strlen(text); i++) {
		char_out(text[i]);
	}
}


/*
 * Here when a keyboard press occurs.
 */
void
do_keystroke(kp)
	GR_EVENT_KEYSTROKE	*kp;
{
	char foo;

	foo = kp->ch;
	write(tfd, &foo, 1);
}


/*
 * Here when a focus in event occurs.
 */
void
do_focusin(gp)
	GR_EVENT_GENERAL	*gp;
{
	if (gp->wid != w1)
		return;
	GrSetBorderColor(w1, LTBLUE);
}

/*
 * Here when a focus out event occurs.
 */
void
do_focusout(gp)
	GR_EVENT_GENERAL	*gp;
{
	if (gp->wid != w1)
		return;
	GrSetBorderColor(w1, GRAY);
}

/*
 * Here to read the shell input file descriptor.
 */
void
do_fdinput()
{
	char	c;

	if (read(tfd, &c, 1) == 1)
		char_out(c);
}


/*
 * Here on an unrecoverable error.
 */
void
errorcatcher(code, name, id)
	GR_ERROR	code;		/* error code */
	GR_FUNC_NAME	name;		/* function name which failed */
	GR_ID		id;		/* resource id */
{
	GrClose();
	fprintf(stderr, "DEMO ERROR: code %d, function %s, resource id %d\n",
		code, name, id);
	exit(1);
}
