/*
 * Telnet for ELKS / TLVC
 *
 * Based on minix telnet client. (c) 2001 Harry Kalogirou
 * <harkal@rainbow.cs.unipi.gr>
 *
 * 17 Dec 2025 Cleaned up, added ^C support. Greg Haerr
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CTRL(c)     ((c) & 0x1f)
#define ESCAPE      CTRL(']')   /* = escape and terminate session */
#define BUFSIZE     1500
#define debug(...)
//#define RAWTELNET             /* test mode for raw telnet without IAC */

/* telnet protocol */
#define IAC         255
#define IAC_SE      240
#define IAC_NOP     241
#define IAC_DataMark 242
#define IAC_BRK     243
#define IAC_IP      244
#define IAC_AO      245
#define IAC_AYT     246
#define IAC_EC      247
#define IAC_EL      248
#define IAC_GA      249
#define IAC_SB      250
#define IAC_WILL    251
#define IAC_WONT    252
#define IAC_DO      253
#define IAC_DONT    254

#define OPT_ECHO      1
#define OPT_SUPP_GA   3
#define OPT_TERMTYPE  24

#define TERMTYPE_SEND 1
#define TERMTYPE_IS   0

#define FALSE       0
#define TRUE        1

#ifdef __linux__
int     DO_echo = TRUE;
int     DO_echo_allowed = TRUE;
int     WILL_terminal_type = FALSE;
int     WILL_terminal_type_allowed = TRUE;
int     DO_suppress_go_ahead = TRUE;
int     DO_suppress_go_ahead_allowed = TRUE;
#else
int     DO_echo = FALSE;
int     DO_echo_allowed = TRUE;
int     WILL_terminal_type = FALSE;
int     WILL_terminal_type_allowed = TRUE;
int     DO_suppress_go_ahead = FALSE;
int     DO_suppress_go_ahead_allowed = TRUE;
#endif

int tcp_fd;
int discard;
char *term_env;
int escape = ESCAPE;
struct termios def_termios;

static int writeall(int fd, char *buffer, int buf_size);
static int process_opt(char *bp, int count);

void
finish()
{
    int     nonblock = 0;
    ioctl(0, FIONBIO, &nonblock);
    tcsetattr(0, TCSANOW, &def_termios);
    exit(0);
}

void
read_keyboard(void)
{
    int     count;
    char    buffer[BUFSIZE];

    count = read(0, buffer, sizeof(buffer));
    if (count <= 0 || buffer[0] == escape) {
        fprintf(stderr, "\r\ntelnet: session terminated\r\n");
        finish();
    }
    if (buffer[0] == CTRL('C'))
        discard = 1;
    else if (buffer[0] == CTRL('O')) {
        discard ^= 1;
        return;
    }
    count = write(tcp_fd, buffer, count);
    if (count < 0) {
        perror("Connection closed");
        finish();
    }
}

void
read_network(void)
{
    char   *bp, *iacptr;
    int     count, optsize;
    char    buffer[BUFSIZE];

    count = read(tcp_fd, buffer, sizeof(buffer));
    if (count <= 0) {
        printf("\r\nConnection closed\r\n");
        finish();
    }
    if (discard)
        return;

#ifdef RAWTELNET
    write(1, buffer, count);
#else
    bp = buffer;
    do {
        iacptr = memchr(bp, IAC, count);
        if (!iacptr) {
            write(1, bp, count);
            count = 0;
            return;
        }
        if (iacptr && iacptr > bp) {
            write(1, bp, iacptr - bp);
            count -= (iacptr - bp);
            bp = iacptr;
            continue;
        }
        if (iacptr) {
            optsize = process_opt(bp, count);
            if (optsize <= 0)
                return;
            bp += optsize;
            count -= optsize;
        }
    } while (count);
#endif
}

int
usage(void)
{
    printf("Usage: telnet [-e exitchar] host <port>\n");
    return 1;
}

int
main(int argc, char **argv)
{
    unsigned short port;
    ipaddr_t ipaddr;
    int     nonblock;
    struct sockaddr_in locadr, remadr;

    if (argc < 2)
        return usage();

    if (*argv[1] == '-') {
        if (*(argv[1] + 1) != 'e')
            return usage();
        if (*argv[2] == '^')
            escape = *(argv[2] + 1);
        else
            escape = *argv[2];
        escape = (escape & 0xdf) - '@';
        argv += 2;
    }
    port = argv[2] ? atoi(argv[2]) : 23;

    tcgetattr(0, &def_termios);
    signal(SIGINT, finish);

    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        perror("telnet");
        return 1;
    }
    locadr.sin_family = AF_INET;
    locadr.sin_port = PORT_ANY;
    locadr.sin_addr.s_addr = INADDR_ANY;
    if (bind(tcp_fd, (struct sockaddr *)&locadr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return 1;
    }
    ipaddr = in_gethostbyname(argv[1]);
    if (!ipaddr) {
        perror(argv[1]);
        return 1;
    }
    remadr.sin_family = AF_INET;
    remadr.sin_port = htons(port);
    remadr.sin_addr.s_addr = ipaddr;
    printf("Trying %s (%s:%u)...\n", argv[1], in_ntoa(ipaddr), port);
    if (in_connect(tcp_fd,
            (struct sockaddr *)&remadr, sizeof(struct sockaddr_in), 10) < 0) {
        perror("Connection failed");
        return 1;
    }
    printf("Connected\n");
    printf("Escape character is '^%c'.\n", escape + '@');

    struct termios termios;
    tcgetattr(0, &termios);
#ifdef RAWTELNET
    termios.c_iflag &= ~(ICRNL | IGNCR | INLCR | IXON | IXOFF);
    termios.c_oflag &= ~(OPOST);
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG);
#else
    termios.c_lflag &= ~ISIG;
#endif
    tcsetattr(0, TCSANOW, &termios);
    nonblock = 1;
    ioctl(0, FIONBIO, &nonblock);

    for (;;) {
        int n;
        fd_set  fdset;
        struct timeval tv;

        FD_ZERO(&fdset);
        FD_SET(0, &fdset);
        FD_SET(tcp_fd, &fdset);
        tv.tv_sec = 0;
        tv.tv_usec = 10000;

        n = select(tcp_fd + 1, &fdset, NULL, NULL, &tv);
        if (n == 0) {
            //if (discard)
                //write(tcp_fd, "\n", 1);
            discard = 0;
            continue;
        }
        if (n < 0) {
            perror("select");
            break;
        }
        if (FD_ISSET(tcp_fd, &fdset))
            read_network();
        if (FD_ISSET(0, &fdset))
            read_keyboard();
    }
    finish();
}

/* telnet protocol handling */
#define next_char(var)                      \
    if (offset < count)                     \
        (var) = bp[offset++];               \
    else                                    \
        read(tcp_fd, (char *)&(var), 1)     \

static void
do_option(int optsrt)
{
    int     result;
    unsigned char reply[3];

    switch (optsrt) {
    case OPT_TERMTYPE:
        if (WILL_terminal_type)
            return;
        if (!WILL_terminal_type_allowed) {
            reply[0] = IAC;
            reply[1] = IAC_WONT;
            reply[2] = optsrt;
        } else {
            WILL_terminal_type = TRUE;
            term_env = getenv("TERM");
            if (!term_env)
                term_env = "unknown";
            reply[0] = IAC;
            reply[1] = IAC_WILL;
            reply[2] = optsrt;
        }
        break;
    default:
        debug("got a DO (%d)\r\n", optsrt);
        debug("WONT (%d)\r\n", optsrt);
        reply[0] = IAC;
        reply[1] = IAC_WONT;
        reply[2] = optsrt;
        break;
    }
    result = writeall(tcp_fd, (char *)reply, 3);
    if (result < 0)
        perror("write");
}

static void
will_option(int optsrt)
{
    int     result;
    unsigned char reply[3];

    switch (optsrt) {
    case OPT_ECHO:
        if (DO_echo)
            break;
        if (!DO_echo_allowed) {
            reply[0] = IAC;
            reply[1] = IAC_DONT;
            reply[2] = optsrt;
        } else {
            struct termios termios;
            tcgetattr(0, &termios);
            termios.c_iflag &= ~(ICRNL | IGNCR | INLCR | IXON | IXOFF);
            /* termios.c_oflag &= ~(OPOST); *//* leave OPOST|ONLCR on */
            termios.c_lflag &= ~(ECHO | ECHONL | ICANON);       /* leave ISIG on for ^P */
            termios.c_cc[VINTR] = 0;    /* turn ^C off */
            termios.c_cc[VSUSP] = 0;    /* turn ^Z off */
            tcsetattr(0, TCSANOW, &termios);
            DO_echo = TRUE;
            reply[0] = IAC;
            reply[1] = IAC_DO;
            reply[2] = optsrt;
        }
        result = writeall(tcp_fd, (char *)reply, 3);
        if (result < 0)
            perror("write");
        break;

    case OPT_SUPP_GA:
        if (DO_suppress_go_ahead)
            break;
        if (!DO_suppress_go_ahead_allowed) {
            reply[0] = IAC;
            reply[1] = IAC_DONT;
            reply[2] = optsrt;
        } else {
            DO_suppress_go_ahead = TRUE;
            reply[0] = IAC;
            reply[1] = IAC_DO;
            reply[2] = optsrt;
        }
        result = writeall(tcp_fd, (char *)reply, 3);
        if (result < 0)
            perror("write");
        break;

    default:
        debug("got a WILL (%d)\r\n", optsrt);
        debug("DONT (%d)\r\n", optsrt);
        reply[0] = IAC;
        reply[1] = IAC_DONT;
        reply[2] = optsrt;
        result = writeall(tcp_fd, (char *)reply, 3);
        if (result < 0)
            perror("write");
        break;
    }
}

static int
writeall(int fd, char *buffer, int buf_size)
{
    int     result;

    while (buf_size) {
        result = write(fd, buffer, buf_size);
        if (result <= 0)
            return -1;
        buffer += result;
        buf_size -= result;
    }
    return 0;
}

static void
dont_option(int optsrt)
{
    switch (optsrt) {
    default:
        debug("got a DONT (%d)\r\n", optsrt);
        break;
    }
}

static void
wont_option(int optsrt)
{
    switch (optsrt) {
    default:
        debug("got a WONT (%d)\r\n", optsrt);
        break;
    }
}

static int
sb_termtype(char *bp, int count)
{
    unsigned char command, iac, optsrt;
    unsigned char buffer[4];
    int     offset, result, ret_value;

    offset = 0;
    next_char(command);
    if (command == TERMTYPE_SEND) {
        buffer[0] = IAC;
        buffer[1] = IAC_SB;
        buffer[2] = OPT_TERMTYPE;
        buffer[3] = TERMTYPE_IS;
        result = writeall(tcp_fd, (char *)buffer, 4);
        if (result < 0) {
            ret_value = result;
            goto ret;
        }
        count = strlen(term_env);
        if (!count) {
            term_env = "unknown";
            count = strlen(term_env);
        }
        result = writeall(tcp_fd, term_env, count);
        if (result < 0) {
            ret_value = result;
            goto ret;
        }
        buffer[0] = IAC;
        buffer[1] = IAC_SE;
        result = writeall(tcp_fd, (char *)buffer, 2);
        if (result < 0) {
            ret_value = result;
            goto ret;
        }
    } else {
        debug("got an unknown command (skipping)\r\n");
    }
    for (;;) {
        next_char(iac);
        if (iac != IAC)
            continue;
        next_char(optsrt);
        if (optsrt == IAC)
            continue;
        if (optsrt != IAC_SE) {
            debug("got IAC %d\r\n", optsrt);
        }
        break;
    }
    ret_value = offset;
ret:
    return ret_value;
}


static int
process_opt(char *bp, int count)
{
    unsigned char iac, command, optsrt, sb_command;
    int     offset, result;

    offset = 0;
    next_char(iac);
    if (iac != IAC)
        return -1;
    next_char(command);
    switch (command) {
    case IAC_NOP:
        break;
    case IAC_DataMark:
        debug("got a DataMark\r\n");
        break;
    case IAC_BRK:
        debug("got a BRK\r\n");
        break;
    case IAC_IP:
        debug("got a IP\r\n");
        break;
    case IAC_AO:
        debug("got a AO\r\n");
        break;
    case IAC_AYT:
        debug("got a AYT\r\n");
        break;
    case IAC_EC:
        debug("got a EC\r\n");
        break;
    case IAC_EL:
        debug("got a EL\r\n");
        break;
    case IAC_GA:
        debug("got a GA\r\n");
        break;
    case IAC_SB:
        next_char(sb_command);
        switch (sb_command) {
        case OPT_TERMTYPE:
            debug("got SB TERMINAL-TYPE\r\n");
            result = sb_termtype(bp + offset, count - offset);
            if (result < 0)
                return result;
            else
                return result + offset;
        default:
            debug("got an unknown SB (skipping)\r\n");
            for (;;) {
                next_char(iac);
                if (iac != IAC)
                    continue;
                next_char(optsrt);
                if (optsrt == IAC)
                    continue;
                if (optsrt != IAC_SE)
                    debug("got IAC %d\r\n", optsrt);
                break;
            }
        }
        break;
    case IAC_WILL:
        next_char(optsrt);
        will_option(optsrt);
        break;
    case IAC_WONT:
        next_char(optsrt);
        wont_option(optsrt);
        break;
    case IAC_DO:
        next_char(optsrt);
        do_option(optsrt);
        break;
    case IAC_DONT:
        next_char(optsrt);
        dont_option(optsrt);
        break;
    case IAC:
        debug("got a IAC\r\n");
        break;
    default:
        debug("got unknown command (%d)\r\n", command);
    }
    return offset;
}
