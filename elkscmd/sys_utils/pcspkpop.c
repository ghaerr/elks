/* pcspkpop - ELKS PC speaker sequencer test: Popcorn opening loop */

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef __KD_MAJ
#define __KD_MAJ        ('K'<<8)
#endif
#ifndef KIOCSOUND
#define KIOCSOUND       (__KD_MAJ + 0x2f)
#endif
#ifndef KIOCSNDSEQ
#define KIOCSNDSEQ      (__KD_MAJ + 0x30)
#define PCSPK_F_REST    0x00
#define PCSPK_F_TONE    0x01
#define PCSPK_F_STOP    0x02
#define PCSPK_F_FLUSH   0x04
#define PCSPK_SEQ_F_FLUSH 0x0001
#define PCSPK_SEQ_F_STOP  0x0002
struct pcspk_event {
    unsigned short divisor;
    unsigned short ticks;
    unsigned char flags;
    unsigned char priority;
};
struct pcspk_seq {
    struct pcspk_event *events;
    unsigned short count;
    unsigned short rate_hz;
    unsigned short flags;
};
#endif

#define REST 0
#define C4   4560
#define D4   4063
#define E4   3620
#define F4   3417
#define A4   2712
#define BB4  2560
#define C5   2280
#define D5   2031
#define E5   1810
#define F5   1708
#define A5   1356

struct note {
    unsigned short div;
    unsigned char dur;
};

static int seqfd = -1;
static volatile int stop_requested;

static void stop_sound(void)
{
    struct pcspk_seq s;

    if (seqfd < 0)
        return;
    s.events = 0;
    s.count = 0;
    s.rate_hz = 0;
    s.flags = PCSPK_SEQ_F_STOP | PCSPK_SEQ_F_FLUSH;
    ioctl(seqfd, KIOCSNDSEQ, &s);
    ioctl(seqfd, KIOCSOUND, 0);
}

static void on_signal(int sig)
{
    stop_requested = sig ? sig : 1;
}

static int send_events(struct pcspk_event *ev, int count, int flush)
{
    struct pcspk_seq s;
    int off = 0;
    int n;

    while (off < count && !stop_requested) {
        s.events = ev + off;
        s.count = count - off;
        s.rate_hz = 0;
        s.flags = flush ? PCSPK_SEQ_F_FLUSH : 0;
        flush = 0;

        n = ioctl(seqfd, KIOCSNDSEQ, &s);
        if (n > 0) {
            off += n;
            continue;
        }
        if (n < 0 && errno != EAGAIN && errno != EBUSY)
            return -1;
        usleep(50000L);
    }

    if (stop_requested) {
        errno = EINTR;
        return -1;
    }
    return 0;
}

static unsigned short tempo_ticks(unsigned char dur, int tempo)
{
    unsigned long v;

    /* dur is in eighth-note units.  ticks = 100Hz * 60s / tempo / 2. */
    v = (unsigned long)dur * 3000UL;
    v = (v + (unsigned)tempo / 2) / (unsigned)tempo;
    if (v < 1)
        v = 1;
    if (v > 1000)
        v = 1000;
    return (unsigned short)v;
}

static int build_events(struct pcspk_event *out, struct note *in, int nnotes,
                        int tempo, int add_stop)
{
    int i;
    int n = 0;
    unsigned short t;
    unsigned short gap;

    for (i = 0; i < nnotes; i++) {
        t = tempo_ticks(in[i].dur, tempo);
        /* gap = (t > 3) ? 1 : 0; */
        gap = (t > 8) ? (t >> 3) : 1;
        if (in[i].div) {
            out[n].divisor = in[i].div;
            out[n].ticks = t - gap;
            out[n].flags = PCSPK_F_TONE;
            out[n].priority = 0;
            n++;
        }
        if (gap || !in[i].div) {
            out[n].divisor = 0;
            out[n].ticks = gap ? gap : t;
            out[n].flags = PCSPK_F_REST;
            out[n].priority = 0;
            n++;
        }
    }

    if (add_stop) {
        out[n].divisor = 0;
        out[n].ticks = 1;
        out[n].flags = PCSPK_F_STOP;
        out[n].priority = 0;
        n++;
    }
    return n;
}

static void play_fallback(struct note *song, int nnotes, int tempo)
{
    int i;
    unsigned short t;

    while (!stop_requested) {
        for (i = 0; i < nnotes && !stop_requested; i++) {
            t = tempo_ticks(song[i].dur, tempo);
            if (song[i].div)
                ioctl(seqfd, KIOCSOUND, song[i].div);
            else
                ioctl(seqfd, KIOCSOUND, 0);
            usleep((unsigned long)t * 10000UL);
            ioctl(seqfd, KIOCSOUND, 0);
        }
    }
}

static void usage(void)
{
    fputs("usage: pcspkpop [-s] [-t tempo]\n", stderr);
}

int main(int argc, char **argv)
{
    static struct note popcorn[] = {
        {D5,1},{C5,1},{D5,1},{A4,1},{F4,1},{A4,1},{D4,2},
        {REST,1},
        {D5,1},{C5,1},{D5,1},{A4,1},{F4,1},{A4,1},{D4,2},
        {REST,1},
        {D5,1}, {E5,1}, {F5,1}, {E5,1}, {F5,1}, {D5,1},
        {E5,1}, {D5,1}, {E5,1}, {C5,1},
        {D5,1}, {C5,1}, {D5,1}, {BB4,1}, {D5,2}
    };
    static struct note scale[] = {
        {C4,1},{D4,1},{E4,1},{F4,1},{A4,1},{C5,1},{D5,1},{E5,1},{F5,1},{A5,2}
    };
    static struct pcspk_event events[96];
    struct note *song = popcorn;
    int nnotes = sizeof(popcorn) / sizeof(popcorn[0]);
    int tempo = 120;
    int i;
    int nevents;
    int flush = 1;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s")) {
            song = scale;
            nnotes = sizeof(scale) / sizeof(scale[0]);
        } else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            tempo = atoi(argv[++i]);
            if (tempo < 30)
                tempo = 30;
            if (tempo > 240)
                tempo = 240;
        } else {
            usage();
            return 1;
        }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    seqfd = open("/dev/tty", 0);
    if (seqfd < 0)
        seqfd = 0;

    nevents = build_events(events, song, nnotes, tempo, 0);

    while (!stop_requested) {
        if (send_events(events, nevents, flush) < 0) {
            if (stop_requested)
                break;
            fputs("pcspkpop: KIOCSNDSEQ unavailable, using KIOCSOUND fallback\n",
                  stderr);
            play_fallback(song, nnotes, tempo);
            break;
        }
        flush = 0;
    }

    stop_sound();
    return 0;
}
