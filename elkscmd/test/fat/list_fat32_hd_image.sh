#!/bin/bash

echo "Listing files on an ELKS FAT Hard Drive Image"
echo

set +x

# Hard drive image file name
HD_IMAGE=fat32_hdd

# Mount point for HD image staging
HD_MOUNT=elks_hdd

if [ $UID != 0 ]
	then echo -e "\nThis script must be run with root permissions\n" >&2
	exit 1
fi

# unmount in case still mounted
umount $HD_IMAGE /mnt/elks 2>/dev/null

# Get the next free loop device
LOOP=$(losetup -f)
test ! -e "$LOOP" && echo "Failed to get a loop device" >&2 && exit 1

# add first partition to the loopback driver
# 32.256 is block 63 times 512 blocksize
# 63*512 = 32256 - 48194*512 = 24675328
losetup -o 32256 --sizelimit 24675328 $LOOP $HD_IMAGE

# make mountpoint for $HD_IMAGE called $HD_MOUNT
mkdir -p /mnt/$HD_MOUNT

# mount first partition as a loop device
mount $LOOP /mnt/$HD_MOUNT || echo "Failed to mount hard drive image" >&2

# show files on the hard disk image
#echo -e "\nGenerated root directory:"
ls -l /mnt/$HD_MOUNT
echo
ls -l /mnt/$HD_MOUNT/sutil

# Take down mounts
umount /mnt/$HD_MOUNT
losetup -d $LOOP
