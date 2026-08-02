/*
 * audiorcv - play raw unsigned 8-bit mono PCM arriving over TCP on /dev/dsp
 *
 * usage: audiorcv [-d] [-p port] [-r rate] [-b bytes] [-4]
 *
 * -4 expects Creative 4-bit ADPCM on the wire and expands it here, which
 * halves what the network has to carry for a given sample rate.  The card
 * has its own ADPCM playback commands, but an OPTi 82C929 decodes them to
 * something other than what Creative's encoder produces (measured: a
 * 440Hz tone came back at 794Hz), so the expansion is done in this
 * program and the driver is fed plain 8-bit PCM.
 *
 * Listens on a TCP port and writes whatever arrives straight to /dev/dsp.  The
 * stream carries no header, so the sample rate is a property of this end: -r
 * must match what the sender was told to produce.  Only raw unsigned 8-bit
 * mono PCM is understood, because that is all the driver accepts.
 *
 *   ELKS:  audiorcv -r 8000
 *   host:  sox in.wav -t raw -r 8000 -c 1 -b 8 -e unsigned - | nc elks 4950
 *          ffmpeg -i in.wav -f u8 -ar 8000 -ac 1 - | nc elks 4950
 *
 * Without -d one stream is played and the exit status says whether it played
 * cleanly, so it can be scripted like play.  With -d the process detaches
 * and serves streams one after another forever, logging to the console.
 *
 * /dev/dsp is opened per stream rather than held open, so play can still
 * use the card while the daemon sits idle.  It is a single-opener device, which
 * is also why connections are served one at a time in this process instead of
 * being forked off: a second opener would only get EBUSY.
 *
 * Playback is real time.  The link has to sustain `rate' bytes per second and
 * the slack is one DMA block (4096 bytes, about half a second at 8000 Hz),
 * because the driver keeps exactly one block in flight.  Ethernet is fine;
 * SLIP at 19200 baud carries around 1900 bytes per second and cannot feed even
 * the 4000 Hz minimum, so a serial link needs 40000 baud or better.  TCP flow
 * control does the throttling, so a slow link breaks the audio up rather than
 * corrupting it, and the underrun count says so.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/soundcard.h>

#define DEF_PORT        4950
#define DEFAULT_RATE    8000L
#define MIN_RATE        4000L       /* driver limits */
#define MAX_RATE        20000L      /* the driver's ceiling, SB_MAX_RATE */
#define MIN_BUFSIZE     64
#define MAX_BUFSIZE     4096        /* == the driver's default DMA block */

static unsigned char buf[MAX_BUFSIZE];
/* Expansion target: a half block of packed data becomes a full block of PCM */
static unsigned char pcm[MAX_BUFSIZE];
static audio_errinfo einfo;         /* 104 bytes: keep off the small stack */

static void usage(void)
{
    fprintf(stderr, "usage: audiorcv [-d] [-p port] [-r rate] [-b bytes] [-4]\n");
    fprintf(stderr, "  -4  the stream is Creative 4-bit ADPCM, expanded here\n");
    fprintf(stderr, "       -d serves streams forever in the background,"
        " otherwise one stream is played\n");
    fprintf(stderr, "       raw unsigned 8-bit mono PCM, %ld-%ld Hz,"
        " default %ld, port %d\n", MIN_RATE, MAX_RATE, DEFAULT_RATE, DEF_PORT);
    exit(1);
}

/*
 * Fill the buffer unless the sender stops first.  A socket read() is clamped
 * well below our buffer size, so without this each DMA transfer would be a
 * fraction of a block and the audio would break up.
 */
static int read_full(int fd, unsigned char *p, int len)
{
    int got = 0;
    int n;

    while (got < len) {
        n = read(fd, p + got, (size_t)(len - got));
        if (n < 0) {
            if (errno == EINTR)     /* ktcp returns this on its own */
                continue;
            return -1;
        }
        if (n == 0)
            break;
        got += n;
    }
    return got;
}

static int write_all(int fd, unsigned char *p, int len)
{
    int n;

    while (len > 0) {
        n = write(fd, p, (size_t)len);
        if (n < 0) {
            if (errno == EINTR)     /* nothing was accepted, safe to retry */
                continue;
            return -1;
        }
        if (n == 0)                 /* no progress, do not spin */
            return -1;
        p += n;
        len -= n;
    }
    return 0;
}

/*
 * Creative 4-bit ADPCM expansion, the encoding DOSBox and 86Box implement.
 * Two output samples per input byte, high nibble first; the first byte of a
 * stream is the reference sample.
 *
 * The published algorithm clamps an index, looks up a delta, then adjusts a
 * step from a second table - too much per sample for a 4.77MHz 8086 to keep
 * ahead of 16000 of them a second.  It collapses though: the step only ever
 * holds four values, so (step, nibble) is 64 states whose delta and next
 * step are precomputed below.  Decoding a nibble is then one indexed byte
 * fetch for the delta, one for the next state, an add and a clamp.
 */
#define ADPCM_STATES    4

static const signed char adpcm_delta[ADPCM_STATES * 16] = {
       0,    1,    2,    3,    4,    5,    6,    7,    0,   -1,   -2,   -3,   -4,   -5,   -6,   -7,
       1,    3,    5,    7,    9,   11,   13,   15,   -1,   -3,   -5,   -7,   -9,  -11,  -13,  -15,
       2,    6,   10,   14,   18,   22,   26,   30,   -2,   -6,  -10,  -14,  -18,  -22,  -26,  -30,
       4,   12,   20,   28,   36,   44,   52,   60,   -4,  -12,  -20,  -28,  -36,  -44,  -52,  -60
};

static const unsigned char adpcm_next[ADPCM_STATES * 16] = {
    0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1,
    0, 1, 1, 1, 1, 2, 2, 2, 0, 1, 1, 1, 1, 2, 2, 2,
    1, 2, 2, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 3, 3, 3,
    2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3
};

static int adpcm;               /* -4: the wire carries 4-bit ADPCM */
static int adpcm_ref = 128;     /* running predictor */
static int adpcm_state;         /* index into the tables above */
static int adpcm_primed;        /* the reference sample has been read */

/*
 * Expand n packed bytes from src into dst, returning the samples written.
 * The reference test is hoisted out of the loop and both buffers are walked
 * with pointers: at 16000 samples a second an 8086 has about 300 cycles per
 * sample for everything the program does, so a per-byte branch and an index
 * multiply are not free.
 */
static int adpcm_expand(unsigned char *dst, unsigned char *src, int n)
{
    unsigned char *out = dst;
    unsigned char *end = src + n;
    const signed char *delta = adpcm_delta;
    const unsigned char *next = adpcm_next;
    int ref = adpcm_ref;
    int base = adpcm_state << 4;

    if (!adpcm_primed && src < end) {   /* first byte of a stream is the ref */
        adpcm_primed = 1;
        ref = *src;
        base = 0;
        *out++ = *src++;
    }
    while (src < end) {
        unsigned char b = *src++;
        int k;

        k = base + (b >> 4);
        ref += delta[k];
        if (ref < 0)
            ref = 0;
        else if (ref > 255)
            ref = 255;
        *out++ = (unsigned char)ref;
        base = next[k] << 4;

        k = base + (b & 0x0F);
        ref += delta[k];
        if (ref < 0)
            ref = 0;
        else if (ref > 255)
            ref = 255;
        *out++ = (unsigned char)ref;
        base = next[k] << 4;
    }
    adpcm_ref = ref;
    adpcm_state = base >> 4;
    return (int)(out - dst);
}

/*
 * Open /dev/dsp and program it for the stream about to arrive.  Returns the
 * descriptor, or -1 with a message already printed.  bufsizep is lowered if
 * the driver's DMA block is smaller than the requested write size.
 */
static int dsp_open(long rate, int *bufsizep)
{
    int dsp;
    int32_t val;

    dsp = open("/dev/dsp", O_WRONLY);
    if (dsp < 0) {
        perror("/dev/dsp");
        return -1;
    }

    /* Does this driver offer 8-bit unsigned at all? */
    val = 0;
    if (ioctl(dsp, SNDCTL_DSP_GETFMTS, &val) == 0 && !(val & DSP_FMT_U8)) {
        fprintf(stderr, "audiorcv: no 8-bit unsigned PCM support\n");
        close(dsp);
        return -1;
    }

    /* Format first: it fixes the meaning of the rate and channel count. */
    val = (int32_t)DSP_FMT_U8;
    if (ioctl(dsp, SNDCTL_DSP_SETFMT, &val) < 0 || val != (int32_t)DSP_FMT_U8) {
        perror("SNDCTL_DSP_SETFMT");
        close(dsp);
        return -1;
    }

    /*
     * SPEED is read-write.  The Sound Blaster time constant quantises the
     * rate, so the driver reports back what it actually programmed; playing
     * anyway just shifts the pitch slightly, which beats refusing to play.
     */
    val = (int32_t)rate;
    if (ioctl(dsp, SNDCTL_DSP_SPEED, &val) < 0) {
        perror("SNDCTL_DSP_SPEED");
        close(dsp);
        return -1;
    }
    if (val != (int32_t)rate)
        fprintf(stderr, "audiorcv: %ld Hz requested, %ld Hz selected\n",
            rate, (long)val);

    val = 1;
    if (ioctl(dsp, SNDCTL_DSP_CHANNELS, &val) < 0 || val != 1) {
        perror("SNDCTL_DSP_CHANNELS");
        close(dsp);
        return -1;
    }

    /* Match the driver's block size if it is smaller than our buffer. */
    val = 0;
    if (ioctl(dsp, SNDCTL_DSP_GETBLKSIZE, &val) == 0 &&
        val > 0 && val < (int32_t)*bufsizep)
        *bufsizep = (int)val;

    return dsp;
}

/*
 * Play one connection to completion.  Returns 0 if the stream played cleanly,
 * 1 if it was cut short or the driver reported underruns.
 */
static int play_stream(int conn, long rate, int bufsize)
{
    int dsp;
    int n, err = 0;

    dsp = dsp_open(rate, &bufsize);
    if (dsp < 0)
        return 1;

    adpcm_primed = 0;           /* each stream carries its own reference */
    for (;;) {
        /* a packed read expands to twice its size, so read half a block */
        n = read_full(conn, buf, adpcm? bufsize / 2: bufsize);
        if (n < 0) {
            perror("audiorcv: read");
            err = 1;
            break;
        }
        if (n == 0)
            break;
        if (adpcm)
            n = adpcm_expand(pcm, buf, n);
        if (write_all(dsp, adpcm? pcm: buf, n) < 0) {
            perror("/dev/dsp");
            err = 1;
            break;
        }
    }

    /* Drain before closing or the last partial block is cut off. */
    if (ioctl(dsp, SNDCTL_DSP_SYNC, (char *)0) < 0)
        perror("SNDCTL_DSP_SYNC");

    if (ioctl(dsp, SNDCTL_DSP_GETERROR, &einfo) == 0 && einfo.play_underruns) {
        fprintf(stderr, "audiorcv: %ld underruns\n",
            (long)einfo.play_underruns);
        err = 1;
    }

    close(dsp);
    return err;
}

/* Detach from the terminal, keeping messages on the console. */
static void daemonize(void)
{
    int fd;
    int pid;

    if ((pid = fork()) == -1) {
        fprintf(stderr, "audiorcv: no more processes\n");
        exit(1);
    }
    if (pid)
        exit(0);
    fd = open("/dev/console", O_RDWR);
    close(STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO)
        close(fd);
    setsid();                   /* create new process group */
    signal(SIGINT, SIG_IGN);
}

int main(int argc, char **argv)
{
    long rate = DEFAULT_RATE;
    long port = DEF_PORT;
    int bufsize = MAX_BUFSIZE;
    int dflag = 0;
    int listen_sock, conn_sock;
    int c, val, err;
    struct sockaddr_in localadr;

    while ((c = getopt(argc, argv, "dp:r:b:4")) != -1) {
        switch (c) {
        case 'd':
            dflag = 1;
            break;
        case 'p':
            port = atol(optarg);        /* atoi would wrap above 32767 */
            if (port < 1 || port > 65535) {
                fprintf(stderr, "audiorcv: port must be 1-65535\n");
                return 1;
            }
            break;
        case '4':
            adpcm = 1;
            break;
        case 'r':
            rate = atol(optarg);
            if (rate < MIN_RATE || rate > MAX_RATE) {
                fprintf(stderr, "audiorcv: rate must be %ld-%ld\n",
                    MIN_RATE, MAX_RATE);
                return 1;
            }
            break;
        case 'b': {
            /* parse wide: atoi wraps at 16 bits and can land back in range */
            long b = atol(optarg);
            if (b < MIN_BUFSIZE || b > MAX_BUFSIZE) {
                fprintf(stderr, "audiorcv: buffer must be %d-%d bytes\n",
                    MIN_BUFSIZE, MAX_BUFSIZE);
                return 1;
            }
            bufsize = (int)b;
            break;
        }
        default:
            usage();
        }
    }
    if (optind < argc)
        usage();

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "audiorcv: network is down\n");
        return 1;
    }

    /* set local port reuse, allows server to be restarted in less than 10 secs */
    val = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0)
        perror("SO_REUSEADDR");

    /* set small listen buffer to save ktcp memory */
    val = SO_LISTEN_BUFSIZ;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, &val, sizeof(int)) < 0)
        perror("SO_RCVBUF");

    memset(&localadr, 0, sizeof(localadr));
    localadr.sin_family = AF_INET;
    localadr.sin_port = htons((unsigned short)port);
    localadr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listen_sock, (struct sockaddr *)&localadr, sizeof(localadr)) < 0) {
        fprintf(stderr, "audiorcv: bind error (may already be running)\n");
        return 1;
    }

    /* one stream plays at a time, so there is nothing to gain from a queue */
    if (listen(listen_sock, 1) < 0) {
        perror("audiorcv: listen");
        return 1;
    }

    if (dflag)
        daemonize();

    for (;;) {
        conn_sock = accept(listen_sock, NULL, NULL);
        if (conn_sock < 0) {
            if (errno == ENOTSOCK)      /* ktcp has gone away */
                return 1;
            continue;
        }

        err = play_stream(conn_sock, rate, bufsize);
        close(conn_sock);

        if (!dflag)                     /* one stream, status says how it went */
            return err;
    }
}
