#!/usr/bin/env bash

# ELKS System Builder
# This build script is also called in main.yml for GitHub Continuous Integration
#
# Usage: ./build.sh [auto [[ext] [[[allimages]]]]]
#   <no args>:  user build: build cross-compiler, menuconfig kernel and standard apps
#   auto        github CI build:: just IBM PC, 8018X, NECV25 kernel and standard apps
#   ext         also build external apps (requires OpenWatcom C installed)
#   allimages   also build all floppy and HD disk images
#
# After building the system once, the following can be used to rebuild the system:
#   $ make clean
#   $ make
#   $ ./buildext.sh all     # optionally build specified external apps (OpenWatcom reqd)
#   $ ./qemu.sh
#
set -e

SCRIPTDIR="$(dirname "$0")"

clean_exit () {
	E="$1"
	test -z $1 && E=0
	if [ $E -eq 0 ]
		then echo "Build script has completed successfully."
		else echo "Build script has terminated with error $E"
	fi
	exit $E
}

# Environment setup

. "$SCRIPTDIR/env.sh"
[ $? -ne 0 ] && clean_exit 1

# Build cross tools if not already

if [ "$1" != "auto" ]; then
	mkdir -p "$CROSSDIR"
	tools/build.sh || clean_exit 1
fi

# Configure all

if [ "$1" = "auto" ]; then
	echo "Invoking 'make defconfig'..."
	make defconfig || clean_exit 2
	echo "Building IBM PC image..."
	#cp ibmpc-1440.config .config
	cp ibmpc-1440-nc.config .config
else
	echo
	echo "Now invoking 'make menuconfig' for you to configure the system."
	echo "The defaults should be OK for many systems, but you may want to review them."
	echo -n "Press ENTER to continue..."
	read
	make menuconfig || clean_exit 2
fi

test -e .config || clean_exit 3

# Clean kernel, user land and image

if [ "$1" != "auto" ]; then
	echo "Cleaning all..."
	make clean || clean_exit 4
	fi

# Build default kernel, user land and image
# Forcing single threaded build because of dirty dependencies (see #273)

echo "Building all..."
make -j1 all || clean_exit 5

if [ "$2" = "ext" ]; then
    echo "Building external applications..."
    ./buildext.sh all || clean_exit 51
fi

# Possibly build all images

if [ "$3" = "allimages" ]; then
	echo "Building FD images..."
	cd image
	make -j1 images-minix images-fat || clean_exit 6
	echo "Building HD images..."
	make -j1 images-hd || clean_exit 61
	cd ..
fi

# Build 8018X kernel and image
if [ "$1" = "auto" ]; then
    echo "Building 8018X image..."
    cp 8018x.config .config
    make kclean || clean_exit 7
    rm elkscmd/basic/*.o
    make -j1 || clean_exit 8
fi

# Build NEC V25 kernel and image
if [ "$1" = "auto" ]; then
    echo "Building NECV 25 image..."
    cp necv25.config .config
    make kclean || clean_exit 7
    rm elkscmd/basic/*.o
    make -j1 || clean_exit 8
fi

# Build PC-98 kernel, some user land files and image
if [ "$1" = "auto" ]; then
    echo "Building PC-98 image..."
    cp pc98-1232.config .config
    make kclean || clean_exit 9
    rm bootblocks/*.o
    rm elkscmd/sys_utils/clock.o
    rm elkscmd/sys_utils/ps.o
    rm elkscmd/sys_utils/meminfo.o
    rm elkscmd/sys_utils/beep.o
    rm elkscmd/basic/*.o
    make -j1 || clean_exit 10
fi

# Success

echo "Target image is in 'image' folder."
clean_exit 0
