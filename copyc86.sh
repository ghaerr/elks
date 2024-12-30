#!/usr/bin/env bash
#
# copyc86.sh - copy native ELKS C86 toolchain, header files and examples to ELKS /root
#
# Usage: ./copyc86.sh
#
set -e

if [ -z "$TOPDIR" ]
  then
    echo "ELKS TOPDIR= environment variable not set, run . env.sh"
    exit
fi

if [ -z "$C86" ]
  then
    echo "C86= environment variable not set, run cd libc; . c86env.sh"
    exit
fi

DEST=$TOPDIR/elkscmd/rootfs_template/root

cd $TOPDIR
cp -p libc/include/c86/*.h      $DEST
cp -p libc/libc86.a             $DEST

cd $C86
cp -p elks-bin/make             $DEST
cp -p elks-bin/cpp86            $DEST
cp -p elks-bin/c86              $DEST
cp -p elks-bin/as86             $DEST
cp -p elks-bin/ld86             $DEST
cp -p elks-bin/ar86             $DEST
cp -p elks-bin/objdump86        $DEST
cp -p elks-bin/disasm86         $DEST
cp -p elks-bin/cpp86            $DEST
cd examples
cp -p *.c *.h                   $DEST
cp -p Makefile.elks             $DEST/Makefile
