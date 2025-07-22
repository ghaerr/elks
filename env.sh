#!/usr/bin/env bash

# Set up the build environment

export TOPDIR="$(realpath $(dirname $BASH_SOURCE))"
echo TOPDIR set to $TOPDIR

export CROSSDIR="$TOPDIR/cross"
echo CROSSDIR set to $CROSSDIR

add_path() {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

add_path "$CROSSDIR/bin"
add_path "$TOPDIR/elks/tools/bin"
echo PATH set to $PATH

# for example MAKEFLAGS="-j$(nproc)" . env.sh
export MAKEFLAGS="$MAKEFLAGS"
echo MAKEFLAGS set to $MAKEFLAGS

unset add_path
