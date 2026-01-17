#!/usr/bin/env bash
#
# cleannx.sh - clean Nano-X binaries from distribution
#
set -e

if [ -z "$TOPDIR" ]
  then
    echo "ELKS TOPDIR= environment variable not set, run . env.sh"
    exit
fi

DEST=$TOPDIR/elkscmd/rootfs_template
rm -f $DEST/bin/nano-x
rm -f $DEST/bin/nx*
rm -f $DEST/lib/nx*
rm -f $DEST/root/*.jpg
