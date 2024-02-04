/* beep - This file is part of the ELKS project
 * authors: Florian Zimmermann et al.
 * Free software "as-is"
 * Takes one optional parameter: the beep duration
 */
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "signal.h"

#if 1
void outb(unsigned short port, unsigned char val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) );
}

unsigned char inb(unsigned short port)
{
    unsigned char ret;
    asm volatile( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
#else
#include "arch/io.h"
#endif

static void beep(int freq)
{
    //Set the PIT to the desired frequency
    unsigned int d = 1193180 / freq;
    outb(0x43, 0xb6);
    outb(0x42, (unsigned int)(d));
    outb(0x42, (unsigned int)(d >> 8));

    //And play the sound using the PC speaker
    unsigned int tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

static void silent()
{
    unsigned int tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
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
