#!/bin/bash

# Build the cross tools for IA16 target

# This script is the entry point for the 'cross tools build' workflow
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
