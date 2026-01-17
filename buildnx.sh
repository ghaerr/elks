#!/usr/bin/env bash

# buildnx.sh - build Nano-X in external/microwindows
# This build script is called in main.yml by GitHub Continuous Integration

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

# Clone and build Nano-X for ELKS

echo "Building Nano-X..."
mkdir -p external
cd external
rm -rf microwindows
git clone https://github.com/ghaerr/microwindows.git
cd microwindows/src
make -f Makefile.elks clean
make -f Makefile.elks

# Success

echo "Nano-X binaries are in $TOPDIR/elkscmd/rootfs_template/bin folder"
clean_exit 0
