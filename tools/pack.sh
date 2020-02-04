#!/bin/bash

# Pack the cross tools

# This script is one entry point for the 'cross tools build' workflow
# See .github/workflow/cross.yml

SCRIPTDIR="$(dirname "$0")"
. "$SCRIPTDIR/env.sh"

export SRCDIR="$TOPDIR/tools"
export DISTDIR="$CROSSDIR/dist"
export BUILDDIR="$CROSSDIR/build"

make -f "$SRCDIR/Makefile" pack
