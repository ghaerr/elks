#!/usr/bin/env bash

# Full clean (except the cross build chain)

SCRIPTDIR="$(dirname "$0")"

# Environment setup

. "$SCRIPTDIR/env.sh"
[ $? -ne 0 ] && exit 1

make -C config clean

rm -f .config
rm -f .config.old

make -C include clean

make clean
