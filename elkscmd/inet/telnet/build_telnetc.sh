#!/bin/sh

# Build Telnet client with GCC for testing

CFLAGS="-O2 -pipe -Wall"

gcc $CFLAGS -c -o ttn_conf.o ttn_conf.c
gcc $CFLAGS -c -o telnet.o telnet.c
gcc $CFLAGS ttn_conf.o telnet.o -o telnetc
