#ifndef __LINUXMT_KD_H
#define __LINUXMT_KD_H

#define __KD_MAJ        ('K'<<8)

/* PC speaker on/off w/frequency */
#define KIOCSOUND       (__KD_MAJ + 0x2f)

/* PC speaker low-rate event sequencer.  Divisor is PIT channel-2 reload. */
#define KIOCSNDSEQ      (__KD_MAJ + 0x30)

#define AUDIO_F_REST    0x00
#define AUDIO_F_TONE    0x01
#define AUDIO_F_STOP    0x02
#define AUDIO_F_FLUSH   0x04

#define AUDIO_SEQ_F_FLUSH       0x0001
#define AUDIO_SEQ_F_STOP        0x0002

#ifndef __ASSEMBLER__
#include <linuxmt/types.h>

struct audio_event {
    unsigned short divisor;     /* PIT channel 2 divisor, 0 = silence */
    unsigned short ticks;       /* duration in kernel sequencer ticks */
    unsigned char flags;        /* AUDIO_F_* */
    unsigned char priority;     /* reserved for simple priority policy */
};

struct audio_seq {
    struct audio_event *events; /* near user pointer in process DS */
    unsigned short count;       /* number of events pointed to by events */
    unsigned short rate_hz;     /* 0 or HZ for this first implementation */
    unsigned short flags;       /* AUDIO_SEQ_F_* */
};

#ifdef __KERNEL__
int audio_seq_ioctl(char *arg);
void audio_seq_stop(void);
void audio_seq_exit(pid_t pid);
#endif

#endif

#endif
