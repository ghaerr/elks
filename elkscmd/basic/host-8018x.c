/*
 * Architecture Specific routines for 8018x
 */
#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "basic.h"
#include "arch/io.h"

/**
 * NOTE: This only works on MY 80C188EB board, needs to be more
 * generalized using the 8018x's GPIOs instead!
 */

static uint8_t the_leds = 0xfe;
void host_digitalWrite(int pin,int state) {
    if (pin > 7) {
        return;
    }

    uint8_t new_state = 1 << pin;
    if (state == 0) {
        the_leds |= new_state;
    } else {
        the_leds &= ~new_state;
    }
    /* there's a latch on port 0x2002 where 8 leds are connected to */
    outb(the_leds, 0x2002);
}

int host_digitalRead(int pin) {
    if (pin > 7) {
        if (pin == 69) {
            /* buffer on port 0x2003, bit #3 is the switch */
            return inb(0x2003) & (1 << 3) ? 1 : 0;
        }
        return 0;
    }

    /* there's a buffer on port 0x2001 where an 8 switches dip switch is conncted to */
    uint8_t b = inb(0x2001);
    uint8_t bit = 1 << pin;

    if (b & bit) {
        return 1;
    }
	return 0;
}
