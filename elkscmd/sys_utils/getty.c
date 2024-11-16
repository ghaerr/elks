/*
 * elkscmd/sys_utils/getty.c
 *
 * Copyright (C) 1998 Alistair Riddoch <ajr@ecs.soton.ac.uk>
 *
 * Source for the /bin/getty command.
 *
 * usage: /bin/getty /dev/tty?? <speed>
 *
 * This file may be distributed under the terms of the GNU General Public
 * License version 2 or, at your option, any later version.
 *
 **************************************************************************
 *
 * This is a small version of getty for use in the ELKS project. It is not
 * fully functional, and may not be the most efficient implementation for
 * larger systems. It minimises memory usage and code size.
 *
 * Support for \? and @? codes has been added in, supporting the following
 * codes:
 *
 *      \@ = @                  @@ = @
 *      \\ = \                  @B = Baud Rate.
 *      \0 = ^@                 @D = Date in dd-mmm-yyy format.
 *      \b = ^H                 @H = System hostname.
 *      \f = ^L                 @L = Line identifier.
 *      \n = ^J                 @S = System Identifier.
 *      \r = ^M                 @T = 24 hour time in HH:MM:SS format.
 *      \s = Space              @U = Users connected.
 *      \t = 8-column tab       @V = Kernel version.
 *
 * Note that @U is not yet implemented, and @V returns a fixed string
 * from the compile-time kernel rather than querying the current kernel.
 * These are all works in progress.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <stdarg.h>
#include <errno.h>
#include <paths.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linuxmt/mem.h>

#define PARSE_ETC_ISSUE 0       /* set =1 to process /etc/issue @ sequences */
#define BOOT_TIMER      1       /* set =1 to display system startup time */
#define DEBUG           0       /* set =1 for debug messages */
#define CONSOLE _PATH_CONSOLE   /* debug messages go here */

#if DEBUG
#define debug           consolemsg
#else
#define debug(...)
#endif

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | ((unsigned int)(off))))

char *  progname;
char    Buffer[64];
int     ch, col;

void consolemsg(const char *str, ...)
{
    static int consolefd = -1;
    char buf[80];

    if (consolefd < 0)
        consolefd = open(CONSOLE, O_RDWR);

    va_list args;
    va_start(args, str);
    sprintf(buf, "%s: ", progname);
    write(consolefd, buf, strlen(buf));
    vsprintf(buf, str, args);
    write(consolefd, buf, strlen(buf));
    va_end(args);
}

#if PARSE_ETC_ISSUE
char    *Date, *Time;

/* Before  = "Sun Dec 25 12:34:56 7890"
 * Columns = "0....:....1....:....2..."
 * After   = "25-Dec-7890 12:34:56 "
 */
void when(void) {
    char *Result;
    time_t now;
    int n;

    if (!Date) {
        now = time(0);
        Result = ctime(&now);

        Result[0]  = Result[8];
        Result[1]  = Result[9];

        Result[3]  = Result[4];
        Result[4]  = Result[5];
        Result[5]  = Result[6];

        Result[7]  = Result[20];
        Result[8]  = Result[21];
        Result[9]  = Result[22];
        Result[10] = Result[23];

        Result[2]  = Result[6]   = '-';

        for (n=20; n>11; n--)
            Result[n] = Result[n-1];

        Result[11] = Result[20] = '\0';

        Date = Result;
        if (*Date < '1')
            Date++;
        Time = Result + 12;
    }
}
#endif

static void put(unsigned char ch)
{
    col++;
    if (ch == '\r' || ch == '\n')
        col = 0;
    write(STDOUT_FILENO, &ch, 1);
}

static void state(char *s)
{
    write(STDOUT_FILENO, s, strlen(s));
}

static speed_t convert_baudrate(speed_t baudrate)
{
        switch (baudrate) {
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        }
        return 0;
}

void show_startup(void)
{
#if BOOT_TIMER
    int fd;
    unsigned offset, kds;
    unsigned short __far *pjiffies;  /* only access low order jiffies word */

    fd = open("/dev/kmem", O_RDONLY);
    if (fd >= 0 && !ioctl(fd, MEM_GETDS, &kds) && !ioctl(fd, MEM_GETJIFFADDR, &offset)) {
        pjiffies = _MK_FP(kds, offset);
        printf("[%u.%02u secs] ", *pjiffies / 100, *pjiffies % 100);
        fflush(stdout);
        close(fd);
    }
#endif
}

int main(int argc, char **argv)
{
    int n, fd;
    speed_t baud = 0;
    struct termios termios;

    progname = argv[0];
    signal(SIGTSTP, SIG_IGN);           /* ignore ^Z stop signal*/

    if (argc < 2 || argc > 3) {
        consolemsg("Usage: %s device [baudrate]\n", argv[0]);
        exit(3);
    }

    if (argc == 2)
        debug("startup args '%s'\n", argv[1]);
    else if (argc == 3) {
        baud = atol(argv[2]);
        debug("startup args '%s' %ld\n", argv[1], baud);
    }

    /* allow execution outside of init*/
    if (getppid() != 1) {
        int tty = open(argv[1], O_RDWR);
        if (tty < 0) {
                consolemsg("cannot open terminal %s\n", argv[1]);
                exit(4);
        }

        debug("redirecting stdio to %s\n", argv[1]);
        close(0); close(1); close(2); /* close inherited stdio */
        if (dup2(tty, 0) != 0 || dup2(tty, 1) != 1 || dup2(tty, 2) != 2) {
                consolemsg("cannot redirect stdio (error %d)\n", errno);
                exit(5);
        }
        close(tty);
    }

    /* setup tty termios state*/
    baud = convert_baudrate(baud);
    if (baud) debug("setting termio baudcode %ld\n", baud);
    if (tcgetattr(STDIN_FILENO, &termios) >= 0) {
        termios.c_lflag |= ISIG | ICANON | ECHO | ECHOE;
        termios.c_lflag &= ~(IEXTEN | ECHOK | NOFLSH | ECHONL);
        termios.c_iflag |= BRKINT | ICRNL;
        termios.c_iflag &= ~(IGNBRK | IGNPAR | PARMRK | INPCK | ISTRIP | INLCR | IGNCR
                | IXON | IXOFF | IXANY);
        termios.c_oflag |= OPOST | ONLCR;
        termios.c_oflag &= ~XTABS;
        if (baud)
            termios.c_cflag = baud;
        termios.c_cflag &= ~PARENB;
        termios.c_cflag |= CS8 | CREAD | HUPCL;
        termios.c_cflag |= CLOCAL;                      /* ignore modem control lines*/
        //termios.c_cflag |= CRTSCTS;           /* hw flow control*/
        termios.c_cc[VMIN] = 1;
        termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);
    } else debug("tcgetattr(0) failed\n");

    fd = open(_PATH_ISSUE, O_RDONLY);
    if (fd >= 0) {
        put('\n');
#if !PARSE_ETC_ISSUE
        while ((n=read(fd,Buffer,sizeof(Buffer))) > 0)
            write(1,Buffer,n);
#else
        char *ptr;
        when();
        *Buffer = '\0';
        while (read(fd,Buffer,1) > 0) {
            ch = *Buffer;
            if (ch == '\\' || ch == '@') {
                Buffer[1] = ch;
                read(fd,Buffer+1,1);
            }
            switch (ch) {
                case '\n':
                    put(ch);
                    break;
                case '\\':
                    ch = Buffer[1];
                    switch(ch) {
                        case '0':                       /* NUL */
                            ch = 0;
                        case '\\':
                        case '@':
                            put(ch);
                            break;
                        case 'b':                       /* BS Backspace */
                            put(8);
                            break;
                        case 'f':                       /* FF Formfeed */
                            put(12);
                            break;
                        case 'n':                       /* LF Linefeed */
                            put(10);
                            break;
                        case 's':                       /* SP Space */
                            put(32);
                            break;
                        case 't':                       /* HT Tab */
                            do {
                                put(' ');
                            } while (col & 7);
                            break;
                        case 'r':                       /* CR Return */
                            ch=13;
                        default:                        /* Anything else */
                            put('\\');
                            put(ch);
                            break;
                    }
                    break;
                case '@':
                    ch = Buffer[1];
                    switch(ch) {
                        case '@':
                            put(ch);
                            break;
                        case 'B':                       /* Baud Rate */
                            if (argc > 2) {
                                state(argv[2]);
                                state(" Baud");
                            } else
                                state("Terminal");
                            break;
                        case 'D':                       /* Date */
                            state(Date);
                            break;
                        case 'H':                       /* Host */
                            if (!(ptr = getenv("HOSTNAME")))
                                ptr = "LocalHost";
                            state(ptr);
                            break;
                        case 'L':                       /* Line used */
                            ptr = rindex(argv[1],'/');
                            if (ptr == NULL)
                                ptr = argv[1];
                            state(ptr);
                            break;
                        case 'S':                       /* System */
                            state("ELKS");
                            break;
                        case 'T':                       /* Time */
                            state(Time);
                            break;
                        case 'V':                       /* Version */
                            state(ELKS_VERSION);
                            break;
                        default:
                            put('@');
                            put(ch);
                            break;
                    }
                    break;
                default:
                    put(ch);
                    break;
            }
            *Buffer = '\0';
        }
#endif
        close(fd);
    }

    show_startup();
    for (;;) {
        state("login: ");
        errno = 0;
        n=read(STDIN_FILENO,Buffer,sizeof(Buffer)-1);
        if (n < 1) {
            debug("read fail on stdin, errno %d\n", errno);
            if (errno == EINTR)
                continue;
            exit(1);
        }
        Buffer[n] = '\0';
        while (n > 0)
            if (Buffer[--n] < ' ')
                Buffer[n] = '\0';
        if (*Buffer) {
            char *nargv[3];

            debug("calling login: %s\n", Buffer);
            nargv[0] = _PATH_LOGIN;
            nargv[1] = Buffer;
            nargv[2] = NULL;
            execv(nargv[0], nargv);
            debug("execv fail\n");
            exit(2);
        }
    }
}
