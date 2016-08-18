/*
 * file:         getpass.c
 * description:  implement the stinky getpass() function
 * author:       Shane Kerr <kerr@wizard.net>
 * copyright:    1998 by Shane Kerr <kerr@wizard.net>, under terms of LPGL
 */

#define WORKING_TERMINAL

#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

char *getpass(char *prompt)
{
    static char result[128];
    FILE *in;
    int in_fd;
#ifdef WORKING_TERMINAL
    struct termios old, new;
#else
    struct termio old, new;
#endif
    int reset_terminal;
    int n;

    /* grab our input device */
    in = fopen("/dev/tty", "r");
    if (in == NULL) {
        in = stdin;
    }

    /* disable echo/break/etc. if possible */
    reset_terminal = 0;
    in_fd = fileno(in);

#ifdef WORKING_TERMINAL
    if (isatty(in_fd)) {
        if (tcgetattr(in_fd, &old) == 0) {
	    new = old;
	    new.c_lflag &= ~(ISIG|ECHO);
	    if (tcsetattr(in_fd, TCSAFLUSH, &new) == 0) {
	        reset_terminal = 1;
	    }
	}
    }
#else
    if (ioctl(in_fd, TCGETA, &old) == 0) {
        new = old;
	new.c_lflag &= ~(ISIG|ECHO);
	if (ioctl(in_fd, TCSETA, &new) == 0) {
	    reset_terminal = 1;
	}
    }
#endif /* WORKING_TERMINAL */

    /* display the prompt */
    fputs(prompt, stderr);

    /* read the input */
    if (fgets(result, sizeof(result)-1, in) == NULL) {
        result[0] = 0;
    } else {
	char * p = strchr(result, '\n');
	if (p) *p = '\0';
    }

    /* reset our terminal, if necessary */
    if (reset_terminal) {
#ifdef WORKING_TERMINAL
        tcsetattr(in_fd, TCSAFLUSH, &old);
#else
	ioctl(in_fd, TCSETA, &old);
#endif
    }

    /* return a pointer to our result string */
    return result;
}

