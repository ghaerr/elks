#!/bin/bash

# Build test program for target
# within a single segment CS=DS=ES=SS

as86 -0 -o ne2k-low.o ne2k-low.s
as86 -0 -o ne2k-phy.o ne2k-phy.s
as86 -0 -o ne2k-mac.o ne2k-mac.s

bcc -ansi -0 -c -o ne2k-main.o ne2k-main.c

ld86 -0 -d -M -o ne2k.bin ne2k-low.o ne2k-phy.o ne2k-mac.o ne2k-main.o | sort -k4 > ne2k.map
