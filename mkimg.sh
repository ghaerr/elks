#!/bin/sh
# Build a fresh ELKS boot image with a guaranteed-contiguous /linux.
# Usage: mkimg.sh [kernel-file] [image-name] [runlevel]
# Fails (exit 1) if /linux ends up fragmented -- the FAT boot sector loads
# consecutive sectors from the first cluster only, so a fragmented kernel
# silently boots with a corrupted .data tail (cost us a full day to find).
set -e
KERNEL=${1:-linux-small}
IMG=${2:-small.img}
RUNLEVEL=${3:-3}
cd /work
export MTOOLS_SKIP_CHECK=1

cp fd1440-fat.img "$IMG"
rm -rf /tmp/root; mkdir /tmp/root
mcopy -s -i fd1440-fat.img ::/bin ::/etc ::/lib ::/sbin ::/root ::/home ::/mnt ::/var ::/tmp /tmp/root/ 2>/dev/null || true
sed -i "s/^id:1:initdefault:/id:$RUNLEVEL:initdefault:/" /tmp/root/etc/inittab

# empty the image but keep its boot sector, then /linux FIRST (contiguous)
mdeltree -i "$IMG" ::/dev ::/bin ::/etc ::/home ::/lib ::/mnt ::/root ::/tmp ::/var ::/sbin 2>/dev/null || true
mattrib -i "$IMG" -r -s -h ::/linux 2>/dev/null || true
mdel -i "$IMG" ::/linux 2>/dev/null || true
mdel -i "$IMG" ::/bootopts 2>/dev/null || true
mcopy -i "$IMG" "$KERNEL" ::/linux
mmd -i "$IMG" ::/dev
mmd -i "$IMG" ::/proc
mcopy -s -i "$IMG" /tmp/root/* ::/

CHAIN=$(mshowfat -i "$IMG" ::/linux)
echo "CHAIN: $CHAIN"
EXTENTS=$(echo "$CHAIN" | tr -cd "<" | wc -c)
if [ "$EXTENTS" -ne 1 ]; then
    echo "FATAL: /linux is FRAGMENTED ($EXTENTS extents) - boot would load garbage as kernel data"
    exit 1
fi
echo "IMAGE OK: $IMG /linux contiguous"
