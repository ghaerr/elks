#This script generates a bootable hard disk image for Qemu. It copies
#the files from the full3 floppy image to this hard disk image and the
#bootblocks and mbr from dev86 to the hard disk image.
#
#This script has to run from the elkscmd directory! Otherwise it will not
#find its files. It also needs the file byte0x01 in the elkscmd directory.
#

#uncomment to see every executed command
#set -x

if [ "$(id -u)" != "0" ]; then
   echo -e "\nThis script must be run with 'sudo'\n" 1>&2
   exit 1
fi

#unmount in case still mounted
umount ./full3 >/dev/null
umount ./full3HD >/dev/null

#generate full3 if not found
if [ ! -f ./full3 ]; then
    make full3
fi

#makes flat hard disk image of 32 MB - sfdisk needs 32.2 MB image for that
echo -e '\nGenerating 32 MB disk image:'
dd if=/dev/zero of=./full3HD bs=1024 count=32200

#put partition table on this disk - id=0x80, *=bootable 1 partition for total disk
echo -e -n '\nMaking partition table:'
echo ',,80,*' | sfdisk --quiet -D full3HD 2>/dev/null

#add first partition to the loopback driver
#32.256 is block 63 times 512 blocksize
#63*512 = 32256 - 48194*512 = 24675328
losetup -o 32256 --sizelimit 24675328 /dev/loop1 full3HD

#make minix file system on this hard disk image partition
echo 'Partition formatted as:'
mkfs.minix -n14 -1 /dev/loop1

#make mountpoint for full3HD called elksHD
mkdir -p /mnt/elksHD

#mount first partition as a loop device
mount /dev/loop1 /mnt/elksHD

#mount full3 floppy as loop device
mount -o loop ./full3 /mnt/elks

#copy all files from the full3 floppy image to the hard disk image full3HD
#parameter -dpR needed for /dev files
shopt -s dotglob
cp -dpR /mnt/elks/* /mnt/elksHD/

#write ROOT_DEV as 0301 (0103 little endian) at position 
#0x1FC=508 into the file boot/linux
#byte0x01 is a file which contains the byte 0x01 only, done 
#with a hex editor
echo 'Patch root device for hard disk:'
dd if=./byte0x01 of=/mnt/elksHD/boot/linux bs=1 count=1 seek=508 conv=notrunc 

#make file to indicate the mounted file system
echo 'The hard disk is mounted' > /mnt/elksHD/HardDisk

#show files on the hard disk image
echo -e '\nRoot directory generated:'
ls /mnt/elksHD
echo ' '

#unmount /mnt/elks again
umount /mnt/elks

#unmount /mnt/elksHD again
umount /mnt/elksHD
losetup -d /dev/loop1

#write hard disk boot block to sector 0 in partition 1 of the hard disk image
#this does not enable to boot though yet
echo 'Writing bootblock:'
dd if=../dev86/bootblocks/minixhd.bin of=./full3HD seek=63 bs=512 \
count=2 conv=notrunc 

#write mbr.bin to disk sector zero. Length is 0x1B5 or 437dec = 19*23
echo -e '\nWriting mbr.bin:'
dd if=../dev86/bootblocks/mbr.bin of=full3HD \
bs=23 count=19 conv=notrunc 

#make user owner of full3HD
usr=$(env | grep SUDO_USER | cut -d= -f 2)
chown $usr:users full3HD
#allow all to read, owner to write
chmod 644 full3HD

set +x
if [ -f ./full3HD ]; then
   echo -e '\nRun the full3HD disk image with:'
   echo -e 'qemu-system-i386 -hda ./full3HD\n'
fi
