/* beep - This file is part of the ELKS project
 * authors: Florian Zimmermann, Takahiro Yamada et al.
 * Free software "as-is"
 *
 * By default (no parameters) one beep is output, 
 * this can be adjusted by using parameters that can be consecutives of each:
 * -f frequency as digit
 * -l length as digit in milliseconds
 * -n (to indicate the next beep)
 *
 * Happy Birthday example: beep -f1059 -l300 -n -f1059 -l200 -n -f1188 -l500 -n -f1059 -l500 -n -f1413 -l500 -n -f1334 -l950 -n -f1059 -l300 -n -f1059 -l200 -n -f1188 -l500 -n -f1059 -l500 -n -f1587 -l500 -n -f1413 -l950 -n -f1059 -l300 -n -f1059 -l200 -n -f2118 -l500 -n -f1781 -l500 -n -f1413 -l500 -n -f1334 -l500 -n -f1188 -l500 -n -f1887 -l300 -n -f1887 -l200 -n -f1781 -l500 -n -f1413 -l500 -n -f1587 -l500 -n -f1413 -l900
 */
#include <autoconf.h>           /* for CONFIG_ options */
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "signal.h"
#include "arch/io.h"
#include "arch/ports.h"

static void beep(long freq)
{
#ifdef CONFIG_ARCH_PC98
    unsigned int d;
    unsigned char __far *sysc;
    sysc = (unsigned char __far *) 0x501;
    if (*sysc & 0x80) {
        //Set the PIT to the desired frequency for 8MHz system
        d = 1996800L / freq;
    } else {
        //Set the PIT to the desired frequency for 5MHz system
        d = 2457600L / freq;
    }
    outb(0x76, TIMER_CMDS_PORT);
    outb((unsigned int)(d), TIMER1_PORT);
    outb((unsigned int)(d >> 8), TIMER1_PORT);

    //And play the sound using the PC speaker
    outb(0x06, PORTC_CONTROL);
#else
    //Set the PIT to the desired frequency
    unsigned int d = 1193180L / freq;
    outb(0xb6, TIMER_CMDS_PORT);
    outb((unsigned int)(d), TIMER2_PORT);
    outb((unsigned int)(d >> 8), TIMER2_PORT);

    //And play the sound using the PC speaker
    unsigned int tmp = inb(SPEAKER_PORT);
    if (tmp != (tmp | 3)) {
        outb(tmp | 3, SPEAKER_PORT);
    }
#endif
}

static void silent()
{
#ifdef CONFIG_ARCH_PC98
    outb(0x07, PORTC_CONTROL);
#else
    unsigned int tmp = inb(SPEAKER_PORT) & 0xFC;
    outb(tmp, SPEAKER_PORT);
#endif
}

void beep_signal(int sig)
{
    switch(sig) {
	case SIGTERM: /* I'm falling */
        case SIGINT:
            silent();
            exit(sig);
    }
}

int main(int ac, char **av)
{
    long duration = 333L;
    long freq = 1000L;
    int nbeep;
    int i = 1;

    do {
        nbeep = 0;
        while (ac >= 2 && !nbeep) {
            if(av[i][0] == '-') {
                switch(av[i][1]) {
                case 'f':                    /* Frequency */
                    freq = atol(&av[i][2]);
                    break;
                case 'l':                    /* Duration in millisecond */
                    duration = atol(&av[i][2]);
                    break;
                case 'n':
                    nbeep = 1;
                    break;
                }
            }
            i++;
            ac--;
        }

        signal(SIGINT,  beep_signal);
        signal(SIGTERM, beep_signal);

        beep(freq);
        usleep(duration * 1000L);
        silent();
    } while (nbeep);

    return 0;
}
