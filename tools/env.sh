#!/bin/bash

# Set the cross build tools environment

SCRIPTDIR="$(dirname "$0")"

export TOPDIR="$(cd "$SCRIPTDIR/.." && pwd)"

echo TOPDIR set to $TOPDIR

if [[ ! -e "$TOPDIR/cross" ]]; then
	echo "ERROR: Missing folder for cross build tools.";
	echo "       Create an empty folder, either at the top of this ELKS tree,"
	echo "       or outside this tree and link it from: $TOPDIR/cross";
	exit 1;
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
