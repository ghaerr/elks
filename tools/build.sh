#!/bin/bash

# Build the tools to cross compile ELKS

SCRIPTDIR="$(dirname "$0")"
TOPDIR="$(cd "$SCRIPTDIR/.." && pwd)"

if [[ ! -h "$TOPDIR/cross" ]]; then
	echo "Missing cross tools folder";
	echo "Create a folder out of this ELKS tree"
	echo "and link it from $TOPDIR/cross";
	exit 1;
fi

export CROSSDIR="$(cd "$TOPDIR/cross" && pwd)"	
echo CROSSDIR set to $CROSSDIR

export SRCDIR="$TOPDIR/tools"

export DISTDIR="$CROSSDIR/dist"
export BUILDDIR="$CROSSDIR/build"

BIN="$CROSSDIR/bin"
if [[ ":$PATH:" != *":$BIN:"* ]]; then
	export PATH="$BIN:${PATH:+"$PATH:"}"
	echo PATH set to $PATH
fi

mkdir -p "$BUILDDIR"
cd "$BUILDDIR"
make -f "$SRCDIR/Makefile"
