#!/bin/bash

set +x
 
echo "Adding nano-X demos to ELKS full3 image"
echo
echo "Enter demo file in bin directory to copy to full3 image as script parameter"
echo

# Catch hidden files
shopt -s dotglob

# Source image file name
SRC_IMAGE=full3

# Mount point for floppy image to copy
FD_MOUNT=elks

if [ $UID != 0 ]
	then echo -e "\nThis script must be run with root permissions\n" >&2
	exit 1
fi

# get into elkscmd directory
cd ../../elkscmd

# unmount in case still mounted
umount -d $SRC_IMAGE /mnt/elks >/dev/null

# generate $SRC_IMAGE if not found
test ! -e $SRC_IMAGE && make $SRC_IMAGE

# Get the next free loop device
LOOP=$(losetup -f)
test ! -e "$LOOP" && echo "Failed to get a loop device" >&2 && exit 1

# mount $SRC_IMAGE floppy as loop device
mount -o loop $SRC_IMAGE /mnt/$FD_MOUNT || \
	echo "Failed to mount floppy image" >&2

# copy all files from the $SRC_IMAGE floppy image to the hard disk image $HD_IMAGE
cp -a ../nano-X/src/bin/$1 /mnt/$FD_MOUNT/usr/bin
#cp ../nano-X/src/profile /mnt/$FD_MOUNT/root/.profile
df /mnt/$FD_MOUNT/
	
	# Take down mounts
umount /mnt/$FD_MOUNT

# make user owner of FD image (if using sudo)
if env | grep -q SUDO_USER
	then
	USERNAME=$(env | grep SUDO_USER | cut -d= -f 2)
	chown $USERNAME:users "$SRC_IMAGE"
fi
# allow all to read, owner to write
chmod 644 "$SRC_IMAGE"

# set +x
test -e "$SRC_IMAGE" && \
	echo -e "\nRun $1 in $SRC_IMAGE with\nqemu.sh\n"

# get back into src directory
cd ../nano-X/src

