#!/usr/bin/env bash
#
# Set up Watcom build environment
#
# Usage: . ./wcenv.sh

# change to OpenWatcom installation root it is full path to OpenWatcom location
# if you use your own OpenWatcom build than it is located in rel subdirectory
export WATCOM=/Users/greg/net/open-watcom-v2/rel

add_path () {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

#add_path "$WATCOM/binl"    # for Linux-32
#add_path "$WATCOM/binl64"  # for Linux-64
add_path "$WATCOM/bino64"   # for macOS

echo PATH set to $PATH
