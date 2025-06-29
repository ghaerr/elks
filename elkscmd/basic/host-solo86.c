/*
 * Architecture Specific routines for Solo/86
 */
#include <stdio.h>
#include <stdlib.h>
#include <arch/io.h>

#include "host.h"
#include "basic.h"

/**
 * TODO
 */

void host_digitalWrite(int pin,int state) {
    // TODO
}

int host_digitalRead(int pin) {
    // TODO
	return 0;
}

int host_analogRead(int pin) {
    // TODO
	return 0;
}

void host_pinMode(int pin,int mode) {
    // TODO
}

void host_mode(int mode) {
    // TODO
}

void host_cls() {
	fprintf(outfile, "\033[H\033[2J");
}

void host_color(int fgc, int bgc) {
    // TODO
}

void host_plot(int x, int y) {
    // TODO
}

void host_draw(int x, int y) {
    // TODO
}

void host_circle(int x, int y, int r) {
    // TODO
}

void host_outb(int port, int value) {
    outb(value, port);
}

void host_outw(int port, int value) {
    outw(value, port);
}

int host_inpb(int port) {
    return inb(port);
}

int host_inpw(int port) {
    return inw(port);
}
