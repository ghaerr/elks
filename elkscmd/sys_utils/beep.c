/* beep - This file is part of the ELKS project
 * authors: Florian Zimmermann et al.
 * Free software "as-is"
 * Takes one optional parameter: the beep duration
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
	case SIGKILL: /* I'm falling */
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
    int i;
    if(ac >= 2) {
         for(i = 1; i < ac; i++) {
              if(av[i][0] == '-') {
                   switch(av[i][1]) {
                   case 'f':                    /* Frequency */
                       freq = atol(&av[i][2]);
                       break;
                   case 'l':                    /* Duration in millisecond */
                       duration = atol(&av[i][2]);
                       break;
                   }
              }
         }
    }

    signal(SIGKILL, beep_signal);
    signal(SIGINT,  beep_signal);
    signal(SIGTERM, beep_signal);

    beep(freq);
    usleep(duration * 1000L);
    silent();

    return 0;
}
