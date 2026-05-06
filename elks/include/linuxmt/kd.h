#ifndef __LINUXMT_KD_H
#define __LINUXMT_KD_H

#define __KD_MAJ        ('K'<<8)
#define KIOCSOUND       (__KD_MAJ + 0x2f)

/* PC speaker low-rate event sequencer.  Divisor is PIT channel-2 reload. */
#define KIOCSNDSEQ      (__KD_MAJ + 0x30)

#define PCSPK_F_REST    0x00
#define PCSPK_F_TONE    0x01
#define PCSPK_F_STOP    0x02
#define PCSPK_F_FLUSH   0x04

#define PCSPK_SEQ_F_FLUSH       0x0001
#define PCSPK_SEQ_F_STOP        0x0002

#ifndef __ASSEMBLER__
#include <linuxmt/types.h>

struct pcspk_event {
    unsigned short divisor;     /* PIT channel 2 divisor, 0 = silence */
    unsigned short ticks;       /* duration in kernel sequencer ticks */
    unsigned char flags;        /* PCSPK_F_* */
    unsigned char priority;     /* reserved for simple priority policy */
};

struct pcspk_seq {
    struct pcspk_event *events; /* near user pointer in process DS */
    unsigned short count;       /* number of events pointed to by events */
    unsigned short rate_hz;     /* 0 or HZ for this first implementation */
    unsigned short flags;       /* PCSPK_SEQ_F_* */
};

#ifdef __KERNEL__
int pcspk_seq_ioctl(char *arg);
void pcspk_seq_stop(void);
void pcspk_seq_exit(pid_t pid);
#endif

#endif

#endif
