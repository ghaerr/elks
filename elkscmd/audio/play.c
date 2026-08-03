/*
 * play - play raw unsigned 8-bit mono PCM on /dev/dsp
 *
 * usage: play [-r rate] [-b bytes] [file]
 *
 * Writes are sized to the driver's DMA block (SNDCTL_DSP_GETBLKSIZE, 4096
 * bytes by default).  The driver keeps one block in flight and write() returns
 * when the previous block has drained, so writing a whole block at a time lets
 * the read() for the next one overlap playback of the current one.  Writing
 * more than a block does not queue more audio, it just blocks for longer, so
 * MAX_BUFSIZE only needs raising if the driver's block size ever grows.
 *
 * Only raw unsigned 8-bit PCM is understood.  Convert on the host first:
 *   sox in.wav -t raw -r 8000 -c 1 -b 8 -e unsigned out.raw
 *   ffmpeg -i in.wav -f u8 -ar 8000 -ac 1 out.raw
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/audio.h>

#define DEFAULT_RATE    8000L
#define MIN_RATE        4000L       /* driver limits */
#define MAX_RATE        20000L      /* the driver's ceiling, SB_MAX_RATE */
#define MIN_BUFSIZE     64
#define MAX_BUFSIZE     4096        /* == the driver's default DMA block */

static unsigned char buf[MAX_BUFSIZE];
static audio_errinfo einfo;         /* 104 bytes: keep off the small stack */

static void usage(void)
{
    fprintf(stderr, "usage: play [-r rate] [-b bytes] [file]\n");
    fprintf(stderr, "       raw unsigned 8-bit mono PCM, %ld-%ld Hz,"
        " default %ld\n", MIN_RATE, MAX_RATE, DEFAULT_RATE);
    fprintf(stderr, "G Keet, 2026\n");
    exit(1);
}

/*
 * Fill the buffer unless end of file arrives first.  Without this a pipe hands
 * back one small block per read(), which would turn into a stream of tiny DMA
 * transfers and break the audio up.
 */
static int read_full(int fd, unsigned char *p, int len)
{
    int got = 0;
    int n;

    while (got < len) {
        n = read(fd, p + got, (size_t)(len - got));
        if (n < 0) {
            if (errno == EINTR)
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

int main(int argc, char **argv)
{
    long rate = DEFAULT_RATE;
    int bufsize = MAX_BUFSIZE;
    int in = STDIN_FILENO;
    int dsp;
    int c, n, err = 0;
    int32_t val;

    while ((c = getopt(argc, argv, "r:b:")) != -1) {
        switch (c) {
        case 'r':
            rate = atol(optarg);        /* atoi would wrap above 32767 */
            if (rate < MIN_RATE || rate > MAX_RATE) {
                fprintf(stderr, "play: rate must be %ld-%ld\n",
                    MIN_RATE, MAX_RATE);
                return 1;
            }
            break;
        case 'b': {
            /* parse wide: atoi wraps at 16 bits and can land back in range */
            long b = atol(optarg);
            if (b < MIN_BUFSIZE || b > MAX_BUFSIZE) {
                fprintf(stderr, "play: buffer must be %d-%d bytes\n",
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

    if (optind < argc && strcmp(argv[optind], "-")) {
        in = open(argv[optind], O_RDONLY);
        if (in < 0) {
            perror(argv[optind]);
            return 1;
        }
    }

    dsp = open("/dev/dsp", O_WRONLY);
    if (dsp < 0) {
        perror("/dev/dsp");
        return 1;
    }

    /* Does this driver offer 8-bit unsigned at all? */
    val = 0;
    if (ioctl(dsp, SNDCTL_DSP_GETFMTS, &val) == 0 && !(val & DSP_FMT_U8)) {
        fprintf(stderr, "play: no 8-bit unsigned PCM support\n");
        return 1;
    }

    /* Format first: it fixes the meaning of the rate and channel count. */
    val = (int32_t)DSP_FMT_U8;
    if (ioctl(dsp, SNDCTL_DSP_SETFMT, &val) < 0 || val != (int32_t)DSP_FMT_U8) {
        perror("SNDCTL_DSP_SETFMT");
        return 1;
    }

    /*
     * SPEED is read-write.  The Sound Blaster time constant quantises the
     * rate, so the driver reports back what it actually programmed; playing
     * anyway just shifts the pitch slightly, which beats refusing to play.
     */
    val = (int32_t)rate;
    if (ioctl(dsp, SNDCTL_DSP_SPEED, &val) < 0) {
        perror("SNDCTL_DSP_SPEED");
        return 1;
    }
    if (val != (int32_t)rate)
        fprintf(stderr, "play: %ld Hz requested, %ld Hz selected\n",
            rate, (long)val);

    val = 1;
    if (ioctl(dsp, SNDCTL_DSP_CHANNELS, &val) < 0 || val != 1) {
        perror("SNDCTL_DSP_CHANNELS");
        return 1;
    }

    /* Match the driver's block size if it is smaller than our buffer. */
    val = 0;
    if (ioctl(dsp, SNDCTL_DSP_GETBLKSIZE, &val) == 0 &&
        val > 0 && val < (int32_t)bufsize)
        bufsize = (int)val;

    for (;;) {
        n = read_full(in, buf, bufsize);
        if (n < 0) {
            perror("read");
            err = 1;
            break;
        }
        if (n == 0)
            break;
        if (write_all(dsp, buf, n) < 0) {
            perror("/dev/dsp");
            err = 1;
            break;
        }
    }

    /* Drain before closing or the last partial block is cut off. */
    if (ioctl(dsp, SNDCTL_DSP_SYNC, (char *)0) < 0)
        perror("SNDCTL_DSP_SYNC");

    if (ioctl(dsp, SNDCTL_DSP_GETERROR, &einfo) == 0 && einfo.play_underruns) {
        fprintf(stderr, "play: %ld underruns\n",
            (long)einfo.play_underruns);
        err = 1;
    }

    close(dsp);
    if (in != STDIN_FILENO)
        close(in);
    return err;
}
