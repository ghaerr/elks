#!/bin/bash

# Make the build chain for cross compiling ELKS

SCRIPTDIR="$(dirname "$0")"
HERE="$(cd "$SCRIPTDIR" && pwd)"

export SRCDIR="$HERE/host"
export DISTDIR="$HERE/dist"
export BUILDDIR="$HERE/build"
export PREFIX="$HERE/host"

BIN="$PREFIX/bin"
if [[ ":$PATH:" != *":$BIN:"* ]]; then
	export PATH="$BIN:${PATH:+"$PATH:"}"
	echo PATH set to $PATH
fi

cd "$BUILDDIR"
make -f "$SRCDIR/Makefile"
