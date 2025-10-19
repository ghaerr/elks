#!/usr/bin/env bash
#
# Set up C86 build environment
#
# Usage: . ./c86env.sh

export C86=/Users/greg/net/8086-toolchain

add_path () {
	if [[ ":$PATH:" != *":$1:"* ]]; then
		export PATH="$1:$PATH"
	fi
}

add_path "$C86/host-bin"

echo PATH set to $PATH
