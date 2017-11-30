#!/bin/bash

# Set the tools environment

SCRIPTDIR="$(dirname "$0")"
TOPDIR="$(cd "$SCRIPTDIR/.." && pwd)"

if [[ ! -h "$TOPDIR/dev86" ]]; then
	echo "Missing link to dev86 folder";
	echo "Install and build dev86 out of this ELKS tree"
	echo "and link it from $TOPDIR/dev86";
	exit 1;
fi

if [[ ! -h "$TOPDIR/cross" ]]; then
	echo "Missing link to cross tools folder";
	echo "Create an empty folder out of this ELKS tree"
	echo "and link it from $TOPDIR/cross";
	exit 1;
fi

export CROSSDIR="$TOPDIR/cross"	

add_path () {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

add_path "$TOPDIR/dev86/bin"
add_path "$CROSSDIR/bin"

echo PATH set to $PATH
