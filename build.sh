#!/bin/bash

# Quick and dirty build script for ELKS

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

# Build environment setup
# TODO: check script status on return
. tools/env.sh

# Check tools
mkdir -p "$CROSSDIR"
test -x "$CROSSDIR/bin/ia16-elf-gcc" || tools/build.sh || clean_exit 1

# Working directory
WD="$(pwd)"

### Clean if asked
if [ "$1" = "clean" ]
	then echo
	echo "Cleaning up. Please wait."
	sleep 1
	for X in config elks elkscmd
		do cd $X; make clean; cd "$WD"
	done
	clean_exit 0
fi

### Configure all (kernel + user land)
echo
echo "Now invoking 'make menuconfig' for you to configure the system."
echo "The defaults should be OK for many systems, but you may want to review them."
pause
make menuconfig || clean_exit 2
test -e .config || clean_exit 3

### Clean all
echo "Cleaning all..."
sleep 1
make clean || clean_exit 4

### Build all
echo "Building all..."
sleep 1
# Forcing single threaded build because of dirty dependencies (see #273)
make -j1 all || clean_exit 5
test -e elks/arch/i86/boot/Image || clean_exit 6

echo "Target image is under 'image'."
clean_exit 0
