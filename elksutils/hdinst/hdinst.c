/* hdinst 0.0.1 Hard Drive Installation Utility
 * Copyright (C) 2002, Memory Alpha Systems.
 * Released under the GNU General Public Licence, version 2 only.
 *
 ******************************************************************************
 *
 * This program is designed to take a working ELKS system (probably created
 * by booting from a boot+root combined floppy or from separate boot and root
 * floppies) and copy that installation verbatim to a partition on the local
 * primary hard drive.
 *
 * Note that this program requires that there is an area of free space of
 * between 8 and 30 Megabytes in size that is entirely located within the
 * first 1,024 cylinders of the hard drive, and that there is a free primary
 * partition entry available for recording this partition in.
 *
 * The basic procedure is as follows:
 *
 *  1.	Use fdisk to create the partition to install in.
 *
 *  2.	Format this partition ready for use.
 *
 *  3.	Mount this partition as /mnt locally.
 *
 *  4.	Copy the kernel image to /mnt/boot/elks.img
 *
 *  5.	Starting with the root directory, recursively copy the contents of
 *	each directory to the same place in /mnt with the exception that
 *	directories under /mnt are not copied.
 *
 *  6.	Write the boot sector code to load this kernel image.
 */

int main( int argc, char **argv ) {

    exit( 1 );
}
