# How to build ?

## Prerequisites

To build ELKS, you need a GNU development environment, including:
- flex
- bison
- texinfo
- libncurses5-dev
- mtools-4.0.23 (for MSDOS/FAT images)
- compress (for compressed man pages; use `sudo apt-get install ncompress`)

## Quickstart

A script is provided to automate the whole build process
(cross tool chain, configuration, kernel, user land and target image),
and make it easier for ELKS newbies:

`./build.sh`

Note: all the scripts must be executed within the top folder of
the ELKS repository as the current one (= TOPDIR).

If you want to clean everything up afterwards (except the cross tool chain):

`./clean.sh`

## Build steps

1- Create a `cross` subfolder:

`mkdir cross`

2- Build the cross tool chain, mainly based on a recent GCC-IA16
(DEV86 including BCC was used for previous versions, but has been
dropped because it was obsolete and no longer maintained):

`tools/build.sh`

Ubuntu 18.04 LTS users: as this step is quite long,
you can download an already built cross folder from here:
https://github.com/elks-org/elks/actions?query=workflow%3Across

3- Set up your environment (PATH, TOPDIR and CROSSDIR):

`. ./env.sh` (note the '.' before the script)

4- Configure the kernel, the user land and the target image format:

`make menuconfig`

5- Build the kernel, the user land and the target image:

`make all`

The target root folder is built in `target`, and depending on your
configuration, that folder is used to create either a floppy disk image
(fd360, fd720, fd1200, fd1440, fd2880), a flat 32MB hard disk image (without MBR),
or a ROM file image into the `image` folder. The image extension is '.img'
and will be in either ELKS (MINIX) or MSDOS (FAT) filesystem format.
Building FAT images requires the 'mtools-4.0.23' package to be installed.

6- Before writing that image on the real medium, you can test it first on QEMU:

`./qemu.sh`

7- You can then modify the configuration or the sources and repeat from the
step 4 after cleaning only the kernel, the user land and the image:

`make clean`

8- One can also build ELKS distribution images for the entire suite of
supported floppy formats and hard disks (with and without MBRs) for both
MINIX and MSDOS FAT format. To create these images, use the following:

`cd image; make images`
