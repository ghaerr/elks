#!/usr/bin/env bash
#
# Set up Watcom build environment
#
# Usage: . ./wcenv.sh

# change to OpenWatcom source tree top
export WATDIR=/Users/greg/net/open-watcom-v2

#export WATCOM=$WATDIR/rel/binl     # for Linux-32
#export WATCOM=$WATDIR/rel/binl64   # for Linux-64
export WATCOM=$WATDIR/rel/bino64    # for macOS

add_path () {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

add_path "$WATCOM"

echo PATH set to $PATH
