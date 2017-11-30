#!/bin/bash

# Build the tools to cross compile ELKS

SCRIPTDIR="$(dirname "$0")"
. "$SCRIPTDIR/env.sh"

export SRCDIR="$TOPDIR/tools"
export DISTDIR="$CROSSDIR/dist"
export BUILDDIR="$CROSSDIR/build"

mkdir -p "$BUILDDIR"
cd "$BUILDDIR"
make -f "$SRCDIR/Makefile"
