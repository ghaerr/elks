#!/bin/bash

SCRIPTDIR="$(dirname "$0")"
HERE="$(cd "$SCRIPTDIR" && pwd)"
export PREFIX="$HERE/install"

BIN="$PREFIX/bin"
if [[ ":$PATH:" != *":$BIN:"* ]]; then
	export PATH="$BIN:${PATH:+"$PATH:"}"
	echo PATH set to $PATH
fi

cd "$HERE"
make all
