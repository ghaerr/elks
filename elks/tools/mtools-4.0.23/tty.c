/*  Copyright 1997,2001,2002,2007-2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include "sysincludes.h"
#include "mtools.h"

static FILE *tty=NULL;
static int notty=0;	
static int ttyfd=-1;
#ifdef USE_RAWTERM
int	mtools_raw_tty = 1;
#else
int	mtools_raw_tty = 0;
#endif

#ifdef USE_RAWTERM
# if defined TCSANOW && defined HAVE_TCSETATTR
/* we have tcsetattr & tcgetattr. Good */
typedef struct termios Terminal;
#  define stty(a,b)        (void)tcsetattr(a,TCSANOW,b)
#  define gtty(a,b)        (void)tcgetattr(a,b)
#  define USE_TCIFLUSH

# elif defined TCSETS && defined TCGETS
typedef struct termios Terminal;
#  define stty(a,b) (void)ioctl(a,TCSETS,(char *)b)
#  define gtty(a,b) (void)ioctl(a,TCGETS,(char *)b)
#  define USE_TCIFLUSH

# elif defined TCSETA && defined TCGETA
typedef struct termio Terminal;
#  define stty(a,b) (void)ioctl(a,TCSETA,(char *)b)
#  define gtty(a,b) (void)ioctl(a,TCGETA,(char *)b)
#  define USE_TCIFLUSH

# elif defined(HAVE_SGTTY_H) && defined(TIOCSETP) && defined(TIOCGETP)
typedef struct sgttyb Terminal;
#  define stty(a,b) (void)ioctl(a,TIOCSETP,(char *)b)
#  define gtty(a,b) (void)ioctl(a,TIOCGETP,(char *)b)
#  define USE_SGTTY
#  define discard_input(a) /**/

# else
/* no way to use raw terminal */
#  warning Cannot use raw terminal code (disabled)
#  undef USE_RAWTERM
# endif

#endif

#ifdef USE_TCIFLUSH
# if defined TCIFLUSH && defined HAVE_TCFLUSH
#  define discard_input(a) tcflush(a,TCIFLUSH)
# else
#  define discard_input(a) /**/
# endif
#endif

#ifdef USE_RAWTERM

static int tty_mode = -1; /* 1 for raw, 0 for cooked, -1 for initial */
static int need_tty_reset = 0;
static int handlerIsSet = 0;

#define restore_tty(a) stty(STDIN,a)


#define STDIN ttyfd
#ifdef future
#define FAIL (-1)
#endif
#define DONE 0
static Terminal in_orig;

/*--------------- Signal Handler routines -------------*/

static void tty_time_out(int dummy UNUSEDP) NORETURN;
static void tty_time_out(int dummy UNUSEDP)
{
	int exit_code;
	signal(SIGALRM, SIG_IGN);
	if(tty && need_tty_reset)
		restore_tty (&in_orig);	
#ifdef future
	if (fail_on_timeout)
		exit_code=SHFAIL;
	else {
		if (default_choice && mode_defined) {
			if (yes_no) {
				if ('Y' == default_choice)
					exit_code=0;
				else
					exit_code=1;
			} else
				exit_code=default_choice-minc+1;
		} else
			exit_code=DONE;
	}
#else
	exit_code = DONE;
#endif
	exit(exit_code);
}

static void cleanup_tty(void)
{ 
	if(tty && need_tty_reset) {
		restore_tty (&in_orig);
		setup_signal();
	}
}

static void set_raw_tty(int mode)
{
	Terminal in_raw;

	if(mode != tty_mode && mode != -1) {
		if(!handlerIsSet) {
			/* Determine existing TTY settings */
			gtty (STDIN, &in_orig);
			need_tty_reset = 1;

			/* Restore original TTY settings on exit */
			atexit(cleanup_tty);
			handlerIsSet = 1;
		}


		setup_signal();
		signal (SIGALRM, tty_time_out);
	
		/* Change STDIN settings to raw */

		gtty (STDIN, &in_raw);
		if(mode) {
#ifdef USE_SGTTY
			in_raw.sg_flags |= CBREAK;
#else
			in_raw.c_lflag &= ~ICANON;
			in_raw.c_cc[VMIN]=1;
			in_raw.c_cc[VTIME]=0;			
#endif
			stty (STDIN, &in_raw);
		} else {
#ifdef USE_SGTTY
			in_raw.sg_flags &= ~CBREAK;
#else
			in_raw.c_lflag |= ICANON;
#endif
			stty (STDIN, &in_raw);
		}
		tty_mode = mode;
		discard_input(STDIN);
	}
}
#endif

FILE *opentty(int mode)
{
	if(notty)
		return NULL;
	if (tty == NULL) {
		ttyfd = open("/dev/tty", O_RDONLY);
		if(ttyfd >= 0) {
			tty = fdopen(ttyfd, "r");
		}
	}
	if  (tty == NULL){
		if ( !isatty(0) ){
			notty = 1;
			return NULL;
		}
		ttyfd = 0;
		tty = stdin;
	}
#ifdef USE_RAWTERM
	if(mtools_raw_tty)
		set_raw_tty(mode);
#endif
	return tty;
}

int ask_confirmation(const char *format, ...)
{
	char ans[10];
	va_list ap;

	if(!opentty(-1))
		return 0;

	while (1) {
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fflush(stderr);
		fflush(opentty(-1));
		if (mtools_raw_tty) {
			ans[0] = fgetc(opentty(1));
			fputs("\n", stderr);
		} else {
			if(fgets(ans,9, opentty(0)) == NULL)
				/* Treat end-of-file as no */
				ans[0] = 'n';
		}
		if (ans[0] == 'y' || ans[0] == 'Y')
			return 0;
		if (ans[0] == 'n' || ans[0] == 'N')
			return -1;
	}
}
