/*
 * Architecture Specific stubs
 */
#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "basic.h"

extern FILE *outfile;

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}

int host_analogRead(int pin) {
	return 0;
}

void host_pinMode(int pin,int mode) {
}

void host_mode(int mode) {
}

void host_cls() {
	fprintf(outfile, "\033[H\033[2J");
}

void host_plot(int x, int y) {
}
