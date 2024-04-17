#!/usr/bin/env bash

# BROKEN SCRIPT
# DEV86 is no more linked to this project
# See issue jbruchon/elks#199

# This script generates a bootable hard disk image for Qemu. It copies
# the files from the fd1440 floppy image to this hard disk image and the
# bootblocks and mbr from dev86 to the hard disk image.
#
# This script has to run from the elkscmd directory! Otherwise it will not
# find its files. It also needs the file byte0x01 in the elkscmd directory.

echo "ELKS Hard Drive Image Builder"

# Catch hidden files
shopt -s dotglob

# Source image file name
SRC_IMAGE=fd1440

# Hard drive image file name
HD_IMAGE=full_hdd

# Mount point for HD image staging
HD_MOUNT=elks_hdd

# Mount point for floppy image to copy
FD_MOUNT=elks

if [ $UID != 0 ]
	then echo -e "\nThis script must be run with root permissions\n" >&2
	exit 1
fi

# Location for boot sector and MBR files
BOOTSECT=bootblocks/minixhd.bin
MBR=bootblocks/mbr.bin

# Don't proceed if required files are missing
if [ ! -e "$BOOTSECT" ]
	then echo "'$BOOTSECT' missing; is dev86 present?" >&2
	exit 1
fi
if [ ! -e "$MBR" ]
	then echo "'$MBR' missing; is dev86 present?" >&2
	exit 1
fi

# unmount in case still mounted
umount $SRC_IMAGE $HD_IMAGE /mnt/elks >/dev/null

# generate $SRC_IMAGE if not found
test ! -e $SRC_IMAGE && make $SRC_IMAGE

# Get the next free loop device
LOOP=$(losetup -f)
test ! -e "$LOOP" && echo "Failed to get a loop device" >&2 && exit 1

# makes flat hard disk image of 32 MB - sfdisk needs 32.2 MB image for that
echo -e "\nGenerating 32 MB disk image:"
dd if=/dev/zero of=$HD_IMAGE bs=1024 count=32200

# put partition table on this disk - id=0x80, *=bootable first partition
echo -en "\nMaking partition table:"
echo ',,80,*' | sfdisk --quiet -D $HD_IMAGE 2>/dev/null

# add first partition to the loopback driver
# run fdisk -l -u $HD_IMAGE to determine start and end of partition
# start is block nr. 63 times 512 blocksize = 32256
# end (sizelimit) is block nr. 48194 times 512 blocksize = 24675328
# i.e. partition does not span entire disk image, just about 24 MB
losetup -o 32256 --sizelimit 24675328 $LOOP $HD_IMAGE

# make minix file system on this hard disk image partition
echo "Partition formatted as "
mkfs.minix -n14 -1 $LOOP

# make mountpoint for $HD_IMAGE called $HD_MOUNT
mkdir -p /mnt/$HD_MOUNT

# mount first partition as a loop device
mount $LOOP /mnt/$HD_MOUNT || echo "Failed to mount hard drive image" >&2

# mount $SRC_IMAGE floppy as loop device
mount -o loop $SRC_IMAGE /mnt/$FD_MOUNT || \
	echo "Failed to mount floppy image" >&2

# copy all files from the $SRC_IMAGE floppy image to the hard disk image $HD_IMAGE
cp -a /mnt/$FD_MOUNT/* /mnt/$HD_MOUNT

# write ROOT_DEV as 0301 (0103 little endian) at position
# 0x1FC=508 into the file boot/linux
echo "Patching root device for hard disk"
echo -en \\x01 | dd of=/mnt/$HD_MOUNT/boot/linux bs=1 count=1 seek=508 conv=notrunc

# show files on the hard disk image
echo -e "\nGenerated root directory:"
ls /mnt/$HD_MOUNT
echo

# Take down mounts
umount /mnt/$FD_MOUNT
umount /mnt/$HD_MOUNT
losetup -d $LOOP

# write hard disk boot block to sector 0 in partition 1 of the image
# this does not enable to boot though yet
echo "Writing bootblock"
dd if="$BOOTSECT" of="$HD_IMAGE" seek=63 bs=512 count=2 conv=notrunc

# write mbr.bin to disk sector zero. Length is 0x1b5 or 437 = 19*23
echo -e "\nWriting master boot record"
dd if="$MBR" of="$HD_IMAGE" bs=23 count=19 conv=notrunc

# make user owner of HD image (if using sudo)
if env | grep -q SUDO_USER
	then
	USERNAME=$(env | grep SUDO_USER | cut -d= -f 2)
	chown $USERNAME:users "$HD_IMAGE"
fi
# allow all to read, owner to write
chmod 644 "$HD_IMAGE"

set +x
test -e "$HD_IMAGE" && \
	echo -e "\nRun $HD_IMAGE with\nqemu-system-i386 -hda $HD_IMAGE\n"
