#!/bin/sh

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
if [ "$UID" != "0" ]
	then echo "WARNING: Disk images can only be built logged in as 'root'"
fi

# A copy of dev86 is REQUIRED to build!
if [ ! -e "./dev86" ]
	then
	echo "ERROR: You must copy or symlink 'dev86' to the root of the"
	echo "       ELKS source tree. If you don't have dev86, you can obtain"
	echo "       a copy at:       https://github.com/jbruchon/dev86"
	echo "Cannot build without dev86 in the source tree. Aborting."
	exit 1
fi

# bcc is required (but has no --version switch, so test for stdio output)
if [ -z "$(bcc 2>&1)" ]
	then
	echo "ERROR: Cannot execute 'bcc'. You must build and install dev86 to"
	echo "       your system before attempting to build ELKS. Aborting."
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
make -j4 || clean_exit 4
test -e arch/i86/boot/Image || clean_exit 4
cd "$WD"


### dev86 verification
echo "Verifying dev86 is built."
sleep 1
cd dev86 || clean_exit 5
make || clean_exit 5
cd "$WD"


### elkscmd build
echo "Building 'elkscmd'"
sleep 1
cd elkscmd || clean_exit 7
make clean
make || clean_exit 7


### Make image files
test $UID -ne 0 && echo "Skipping image file build (not root)." && clean_exit 0
make images.zip || clean_exit 8
make images.tar.gz || clean_exit 8
make images.tar.xz || clean_exit 8
cd "$WD"

echo "Images and image file archives are under 'elkscmd'."
clean_exit 0
