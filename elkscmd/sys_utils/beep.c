/* beep - This file is part of the ELKS project
 * authors: Florian Zimmermann et al.
 * Free software "as-is"
 * Takes one optional parameter: the beep duration
 */
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "signal.h"
#include "arch/io.h"

static void beep(int freq)
{
    //Set the PIT to the desired frequency
    unsigned int d = 1193180 / freq;
    outb(0xb6, 0x43);
    outb((unsigned int)(d), 0x42);
    outb((unsigned int)(d >> 8), 0x42);

    //And play the sound using the PC speaker
    unsigned int tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(tmp | 3, 0x61);
    }
}

static void silent()
{
    unsigned int tmp = inb(0x61) & 0xFC;
    outb(tmp, 0x61);
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
    if(ac >= 2) {
        duration = atoi(av[1]);
    }

    signal(SIGKILL, beep_signal);
    signal(SIGINT,  beep_signal);
    signal(SIGTERM, beep_signal);

    beep(1000);
    usleep(duration * 1000L);
    silent();

    return 0;
}
