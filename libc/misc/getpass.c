/*
 * file:         getpass.c
 * description:  implement the stinky getpass() function
 * author:       Shane Kerr <kerr@wizard.net>
 * copyright:    1998 by Shane Kerr <kerr@wizard.net>, under terms of LPGL
 */

#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <paths.h>

char *getpass(char *prompt)
{
    FILE *in;
    int in_fd;
    int reset_terminal;
    struct termios old, new;
    static char result[128];

    /* grab our input device */
    in = fopen(_PATH_TTY, "r");
    if (in == NULL) {
        in = stdin;
    }

    /* disable echo/break/etc. if possible */
    reset_terminal = 0;
    in_fd = fileno(in);

    if (isatty(in_fd)) {
        if (tcgetattr(in_fd, &old) == 0) {
            new = old;
            new.c_lflag &= ~(ISIG|ECHO);
            if (tcsetattr(in_fd, TCSAFLUSH, &new) == 0) {
                reset_terminal = 1;
            }
        }
    }

    /* display the prompt */
    fputs(prompt, stderr);

    /* read the input */
    if (fgets(result, sizeof(result), in) == NULL) {
        result[0] = 0;
    } else {
        char * p = strchr(result, '\n');
        if (p) *p = '\0';
    }

    /* reset our terminal, if necessary */
    if (reset_terminal)
        tcsetattr(in_fd, TCSAFLUSH, &old);

    if (in != stdin)
        fclose(in);

    /* return a pointer to our result string */
    return result;
}
