#!/usr/bin/env bash
#
# copyc86.sh - copy native ELKS C86 toolchain, header files and examples to ELKS /usr
# Also creates $TOPDIR/devc86.tar development tarball
#
# Usage: ./copyc86.sh; cd image; make hd32-minix
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

# create and copy to /usr/bin, /usr/lib, /usr/include, /usr/src on destination
DEST=$TOPDIR/elkscmd/rootfs_template/usr
rm -rf $DEST
rm -rf $TOPDIR/target/usr
mkdir -p $DEST
mkdir -p $DEST/bin
mkdir -p $DEST/lib
mkdir -p $DEST/include
mkdir -p $DEST/include/sys
mkdir -p $DEST/include/linuxmt
mkdir -p $DEST/include/arch
mkdir -p $DEST/include/c86
mkdir -p $DEST/src

cd $TOPDIR
cp -p libc/include/alloca.h                 $DEST/include
cp -p libc/include/ctype.h                  $DEST/include
cp -p libc/include/errno.h                  $DEST/include
cp -p libc/include/fcntl.h                  $DEST/include
cp -p libc/include/features.h               $DEST/include
cp -p libc/include/limits.h                 $DEST/include
cp -p libc/include/malloc.h                 $DEST/include
cp -p libc/include/signal.h                 $DEST/include
cp -p libc/include/stdint.h                 $DEST/include
cp -p libc/include/stdlib.h                 $DEST/include
cp -p libc/include/stdio.h                  $DEST/include
cp -p libc/include/string.h                 $DEST/include
cp -p libc/include/termios.h                $DEST/include
cp -p libc/include/time.h                   $DEST/include
cp -p libc/include/unistd.h                 $DEST/include
cp -p libc/include/sys/cdefs.h              $DEST/include/sys
cp -p libc/include/sys/ioctl.h              $DEST/include/sys
cp -p libc/include/sys/select.h             $DEST/include/sys
cp -p libc/include/sys/stat.h               $DEST/include/sys
cp -p libc/include/sys/types.h              $DEST/include/sys
cp -p libc/include/c86/*.h                  $DEST/include/c86

cp -p elks/include/linuxmt/errno.h          $DEST/include/linuxmt
cp -p elks/include/linuxmt/fcntl.h          $DEST/include/linuxmt
cp -p elks/include/linuxmt/ioctl.h          $DEST/include/linuxmt
cp -p elks/include/linuxmt/limits.h         $DEST/include/linuxmt
cp -p elks/include/linuxmt/types.h          $DEST/include/linuxmt
cp -p elks/include/linuxmt/posix_types.h    $DEST/include/linuxmt
cp -p elks/include/linuxmt/signal.h         $DEST/include/linuxmt
cp -p elks/include/linuxmt/stat.h           $DEST/include/linuxmt
cp -p elks/include/linuxmt/termios.h        $DEST/include/linuxmt
cp -p elks/include/linuxmt/time.h           $DEST/include/linuxmt
cp -p elks/include/arch/cdefs.h             $DEST/include/arch
cp -p elks/include/arch/divmod.h            $DEST/include/arch
cp -p elks/include/arch/posix_types.h       $DEST/include/arch
cp -p elks/include/arch/stat.h              $DEST/include/arch
cp -p elks/include/arch/types.h             $DEST/include/arch

cp -p libc/libc86.a                         $DEST/lib

cd $C86
cp -p elks-bin/make             $DEST/bin
cp -p elks-bin/cpp              $DEST/bin
cp -p elks-bin/c86              $DEST/bin
cp -p elks-bin/as               $DEST/bin
cp -p elks-bin/ld               $DEST/bin
cp -p elks-bin/ar               $DEST/bin
cp -p elks-bin/objdump          $DEST/bin
cp -p elks-bin/disasm86         $DEST/bin
cd examples
cp -p *.c *.h *.s               $DEST/src
cp -p Makefile.elks             $DEST/src/Makefile

cd $TOPDIR/elkscmd/rootfs_template
tar cf $TOPDIR/devc86.tar usr
echo "Files copied and devc86.tar produced"
