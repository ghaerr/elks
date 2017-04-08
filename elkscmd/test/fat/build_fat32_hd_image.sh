#!/bin/bash

echo "ELKS FAT Hard Drive Image Builder"
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
umount -d $HD_IMAGE /mnt/elks 2>/dev/null

# Get the next free loop device
LOOP=$(losetup -f)
test ! -e "$LOOP" && echo "Failed to get a loop device" >&2 && exit 1

# makes flat hard disk image of 32 MB - sfdisk needs 32.2 MB image for that
echo -e "\nGenerating 32 MB disk image:"
dd if=/dev/zero of=$HD_IMAGE bs=1024 count=32200

# put partition table on this disk - id=0x80, *=bootable first partition
echo -en "\nMaking partition table:"
echo ',,0C,*' | sfdisk --quiet -D $HD_IMAGE 2>/dev/null

# add first partition to the loopback driver
# 32.256 is block 63 times 512 blocksize
# 63*512 = 32256 - 48194*512 = 24675328
losetup -o 32256 --sizelimit 24675328 $LOOP $HD_IMAGE

# make minix file system on this hard disk image partition
echo "Partition formatted as "
mkfs.vfat -F 32 $LOOP
#mkfs.msdos -F 32 $LOOP
#mkfs.msdos -F 16 $LOOP

# make mountpoint for $HD_IMAGE called $HD_MOUNT
mkdir -p /mnt/$HD_MOUNT

# mount first partition as a loop device
mount $LOOP /mnt/$HD_MOUNT || echo "Failed to mount hard drive image" >&2

# copy some files to the hard disk image $HD_IMAGE
#this does not work yet!
cp -r ../../sys_utils /mnt/$HD_MOUNT/sutil
#cp -r ../../file_utils /mnt/$HD_MOUNT

#make files in root directory
echo 'This is the FATDISK1 file' > /mnt/$HD_MOUNT/FATDISK1
echo 'This is the FATDISK2 file' > /mnt/$HD_MOUNT/FATDISK2

mkdir /mnt/$HD_MOUNT/testdir
echo 'This is the tfile in the testdir' > /mnt/$HD_MOUNT/testdir/tfile
mkdir /mnt/$HD_MOUNT/testdir/subdir
echo 'This is the sfile in the subdir' > /mnt/$HD_MOUNT/testdir/subdir/sfile

# show files on the hard disk image
echo -e "\nGenerated root directory:"
ls /mnt/$HD_MOUNT
echo

# Take down mounts
umount -d /mnt/$HD_MOUNT
losetup -d $LOOP

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
	echo -e "\nRun $HD_IMAGE with qemu.sh\n"
