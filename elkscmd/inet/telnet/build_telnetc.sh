#!/bin/sh

# Build Telnet client with GCC for testing

CFLAGS="-O2 -pipe -Wall"

gcc $CFLAGS -c -o ttn_conf.o ttn_conf.c
gcc $CFLAGS -c -o ttn.o ttn.c
gcc $CFLAGS ttn_conf.o ttn.o -o telnetc
