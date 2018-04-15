#!/bin/bash

# Set up the build environment

# Must be executed with top directory /elks as the current one

if [[ ! -e "tools/env.sh" ]]; then
	echo "ERROR: You did not sourced this script from the top directory.";
	echo "       Set the top directory /elks as the current one,";
	echo "       then source this script again.";
	return 1;
fi

export TOPDIR="$(pwd)"

echo TOPDIR set to $TOPDIR

if [[ ! -e "$TOPDIR/cross" ]]; then
	echo "ERROR: Missing folder for cross build tools.";
	echo "       Create an empty folder, either at the top of this ELKS tree,"
	echo "       or outside this tree and link it from: $TOPDIR/cross";
	return 1;
fi

export CROSSDIR="$TOPDIR/cross"	

echo CROSSDIR set to $CROSSDIR

add_path () {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

add_path "$CROSSDIR/bin"

echo PATH set to $PATH
