#!/bin/bash

# Quick and dirty build script for ELKS
# Probably buggy, but makes from-scratch builds easier

DELAY=7

pause () {
	echo
	# 'read -n/-s' are bashisms, so work around them
	if [ -z "$BASH_VERSION" ]
		then
		echo "Starting in $DELAY seconds."
		sleep $DELAY
		else
		echo -n "Press a key to continue... "
		read -n 1 -s 2>/dev/null || sleep $DELAY
fi
	echo
}

clean_exit () {
	E="$1"
	test -z $1 && E=0
	if [ $E -eq 0 ]
		then echo "Build script has terminated successfully."
		else echo "Build script has terminated with error $E"
	fi
	exit $E
}

# Disk images cannot be built unless we're UID 0
if [ "$UID" != "0" ] && [ "$1" != "clean" ]
	then echo -e "\nWARNING: Disk images can only be built if you have root permissions"
fi

# Cross build tools environment setup
pushd tools > /dev/null
. ./env.sh
popd > /dev/null

# Check tools
if [ ! -x "$CROSSDIR/bin/bcc" ]
	then
	echo "ERROR: BCC not found. You must build the cross tools"
	echo "       before attempting to build ELKS. Run tools/build.sh"
	echo "       to automatically build that cross tools. Aborting."
	exit 1
fi

# Working directory
WD="$(pwd)"


### Clean if asked
if [ "$1" = "clean" ]
	then echo
	echo "Cleaning up. Please wait."
	sleep 1
	for X in elks elkscmd
		do cd $X; make clean; cd "$WD"
	done
	clean_exit 0
fi

# CPU count detection (Linux w/sysfs only) for parallel make
THREADS=$(test -e /sys/devices/system/cpu/cpu0 && echo /sys/devices/system/cpu/cpu? 2>/dev/null | wc -w)
THREADS=$(echo "$THREADS" | sed 's/[^0-9]//g')
test -z "$THREADS" && THREADS=2
test $THREADS -lt 1 && THREADS=2
THREADS=$((THREADS * 2))
# Allow passing -j1 to force single-threaded make
test "$1" = "-j1" && THREADS=1 
echo "Using $THREADS threads for make"

### Kernel build
echo
echo "Preparing to build the ELKS kernel. This will invoke 'make menuconfig'"
echo "for you to configure the system. The defaults should be OK for many"
echo "systems, but you may want to review them."
pause
cd elks || clean_exit 1
make clean
make menuconfig || clean_exit 2
test -e .config || clean_exit 3
make defconfig || clean_exit 4
make -j$THREADS || clean_exit 4
test -e arch/i86/boot/Image || clean_exit 4
cd "$WD"


### elkscmd build
echo "Building 'elkscmd'"
sleep 1
cd elkscmd || clean_exit 7
make clean
make -j$THREADS || clean_exit 7


### Make image files
test $UID -ne 0 && echo "Skipping image file build (not root)." && clean_exit 0
make images.zip || clean_exit 8
make images.tar.gz || clean_exit 8
make images.tar.xz || clean_exit 8
cd "$WD"

echo "Images and image file archives are under 'elkscmd'."
clean_exit 0
