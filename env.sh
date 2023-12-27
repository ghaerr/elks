#!/usr/bin/env bash

# Set up the build environment

# Must be executed with top directory /elks as the current one

if [ ! -e "env.sh" ]; then
	echo "ERROR: You did not source this script from the top directory.";
	echo "       Set the top directory of ELKS as the current one,";
	echo "       then source this script again.";
	return 1;
fi

export TOPDIR="$(pwd)"
echo TOPDIR set to $TOPDIR

export CROSSDIR="$TOPDIR/cross"	
echo "CROSSDIR set to $CROSSDIR"

add_path () {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

add_path "$CROSSDIR/bin"

# Set up internal ELKS tools path

ELKSTOOLSDIR="$TOPDIR/elks/tools"

add_path "$ELKSTOOLSDIR/bin"

echo PATH set to $PATH

# May inject some Make options

export MAKEFLAGS="$MAKEFLAGS"

echo MAKEFLAGS set to $MAKEFLAGS
