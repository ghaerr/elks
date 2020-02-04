#!/bin/bash

# Build the tools to cross compile ELKS

# This script is the entry point for the 'Cross tools build' workflow
# See .github/workflow/cross.yml

SCRIPTDIR="$(dirname "$0")"
. "$SCRIPTDIR/env.sh"

export SRCDIR="$TOPDIR/tools"
export DISTDIR="$CROSSDIR/dist"
export BUILDDIR="$CROSSDIR/build"

mkdir -p "$DISTDIR"
mkdir -p "$BUILDDIR"

cd "$BUILDDIR"
make -f "$SRCDIR/Makefile"
