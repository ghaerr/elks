#!/bin/sh

# Build Mingw (Windows) executable
#
# For this you meed the mingw cross-compiler to be installed
#
# You may download the RPMs from http://mirzam.it.vu.nl/mingw/
# All 4 RPM's are needed:
#  mingw-binutils
#  mingw-gcc-core
#  mingw-runtime
#  mingw-w32api

dir=`dirname $0`
$dir/configure --srcdir $dir  --host i586-mingw32msvc --disable-floppyd
make
mv mtools mtools.exe
