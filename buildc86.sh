#!/usr/bin/env bash
#
# buildc86.sh - build host and native ELKS C86 toolchain
#
# Requires that the ELKS, WATCOM and C86 variables have been setup using:
#   cd ELKS; . env.sh                   (sets TOPDIR=)
#   cd libc; . wcenv.sh; . c86env.sh    (sets WATCOM= and C86=)
#
# Usage: ./buildc86.sh
#
set -e

if [ -z "$TOPDIR" ]
  then
    echo "ELKS TOPDIR= environment variable not set, run . env.sh"
    exit
fi

if [ -z "$WATCOM" ]
  then
    echo "OpenWatcom WATCOM= environment variable not set, run cd libc; . wcenv.sh"
    exit
fi

if [ -z "$C86" ]
  then
    echo "C86= environment variable not set, run cd libc; . c86env.sh"
    exit
fi

# build ELKS
#cd $ELKS
#make

# build c86 cross compiler toolchain using OWC and GCC in 8086-toolchain/host-bin
cd $C86
make clean
make host

# build native OWC library libc/libc.a usin OWC
cd $TOPDIR
make owc
cd $C86

# build ELKS Version of c86 using host OWC and native OWC libc in 8086-toolchain/elks-bin
make elks

# build native c86 library libc/libc86.a using host c86
cd $TOPDIR
make c86

# build ELKS example apps using host c86 and native c86 libc in 8086-toolchain/examples
cd $C86/examples
make 
