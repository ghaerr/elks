#!/bin/bash

# This script generates a bootable hard disk image for Qemu. It copies
# the files from the fd1440.bin floppy image to this hard disk image and 
# the files minixhd.bin, mbr.bin and boot from elkscmd/bootblocks to the
# hard disk image. The latter files are generated with DEV86.
#
# This script has to run from the elks/image directory! Otherwise it will not
# find its files. 

echo "ELKS Hard Drive Image Builder"

#set -x

# Catch hidden files
shopt -s dotglob

SRC_IMAGE=fd1440.bin
HD_IMAGE=hd.img
FD_MOUNT=elks
HD_MOUNT=elks_hdd

if [ $UID != 0 ]
	then echo -e "\nThis script must be run with root permissions\n" >&2
	exit 1
fi

# Location for boot sector and MBR files
BOOTSECT=../elkscmd/bootblocks/minixhd.bin
MBR=../elkscmd/bootblocks/mbr.bin
BOOTFILE=../elkscmd/bootblocks/boot

# Don't proceed if required files are missing
if [ ! -e "$SRC_IMAGE" ]
	then echo "'$BOOTSECT' missing!"
	exit 1
fi
if [ ! -e "$BOOTSECT" ]
	then echo "'$BOOTSECT' missing!"
	exit 1
fi
if [ ! -e "$MBR" ]
	then echo "'$MBR' missing!"
	exit 1
fi
if [ ! -e "$BOOTFILE" ]
	then echo "'$BOOTFILE' missing!"
	exit 1
fi

# unmount in case still mounted
umount /mnt/$FD_MOUNT >/dev/null
umount /mnt/$HD_MOUNT >/dev/null

# Get the next free loop device
#LOOP=$(losetup -f)
#test ! -e "$LOOP" && echo "Failed to get a loop device" && exit 1

# makes flat hard disk image of 32 MB - sfdisk needs 32.2 MB image for that
echo -e "\nGenerating 32 MB disk image:"
dd if=/dev/zero of=$HD_IMAGE bs=1024 count=32200

# put partition table on this disk - id=0x80, *=bootable first partition
# sfdisk version 2.26 does not support -D option any more use "-u S"
echo -en "\nMaking partition table:"
echo ',,80,*' | sfdisk --quiet -u S $HD_IMAGE 

#add first partition to the loopback driver
#1.048.576 is block 2048 times 512 blocksize
losetup -o 1048576 --sizelimit 32767488 /dev/loop1 $HD_IMAGE
#makes minix file system on this hard disk image partition
mkfs.minix -n14 -1 /dev/loop1
#make mountpoint for hard disk image partition
mkdir -p /mnt/$HD_MOUNT
#mount first partition as a loop device
mount /dev/loop1 /mnt/$HD_MOUNT
if [ $? -ne 0 ]; then
    echo
    echo Minix file system not available or blacklisted
    echo Use modprobe -v minix to load
    exit 1
fi

#mount floppy image as loop device
mount -o loop ./$SRC_IMAGE /mnt/$FD_MOUNT || \
	echo "Failed to mount floppy image" 

#cp all files from the full3 floppy image to the hard disk image full3HD
# -dpR needed for /dev files
cp -dpR /mnt/$FD_MOUNT/* /mnt/$HD_MOUNT/
mkdir /mnt/$HD_MOUNT/boot
mv /mnt/$HD_MOUNT/linux /mnt/$HD_MOUNT/boot
cp $BOOTFILE /mnt/$HD_MOUNT/boot

# write ROOT_DEV as 0301 (0103 little endian) at position
# 0x1FC=508 into the file boot/linux
echo "Patching root device for hard disk"
echo -en \\x01 | dd of=/mnt/$HD_MOUNT/boot/linux bs=1 count=1 seek=508 conv=notrunc

#show files generated on the hard disk image
ls /mnt/$HD_MOUNT
ls /mnt/$HD_MOUNT/boot

umount /mnt/$FD_MOUNT
umount /mnt/$HD_MOUNT
losetup -d /dev/loop1

#write hard disk boot block to sector 0 in partition 1 of the hard disk image
dd if=$BOOTSECT of=./$HD_IMAGE seek=2048 bs=512 count=2 conv=notrunc 
#write mbr into disk sector zero. Length of 0x1B5==437 = 19*23
dd if=$MBR of=./$HD_IMAGE bs=23 count=19 conv=notrunc 

# make user owner of HD image (if using sudo)
if env | grep -q SUDO_USER
	then
	USERNAME=$(env | grep SUDO_USER | cut -d= -f 2)
	chown $USERNAME:users "$HD_IMAGE"
fi

# allow all to write - qemu requires this
chmod 666 "$HD_IMAGE"

set +x
test -e "$HD_IMAGE" && \
	echo -e "\nRun $HD_IMAGE with\nqemu-system-i386 -hda $HD_IMAGE\n"
