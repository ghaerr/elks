# How to build ?

## Prerequisites

To build ELKS, you need a development environment on Linux or macOS or Windows with [WSL](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux), including:
- libncurses5-dev
- ncompress (for compressed man pages; use `sudo apt-get install ncompress`)
- texinfo (not the same as texTinfo)
- libelf-dev
- bison
- flex

On Ubuntu these can be installed with: `sudo apt-get install texinfo libncurses5-dev libelf-dev ncompress bison flex`. Additionally GCC and other common tools are installed with: `sudo apt install build-essential`

## Build output

The build process, depending on the configuration, will produce an ELKS distribution in TOPDIR/target (16 bit kernel, programs, etc). Again, depending on the configuration, a bootable image or images will be produced in TOPDIR/target. These can be booted in QEMU or real hardware. See details below.

## Quick start

Clone this repository. It contains a script that automates the whole build process
(cross tool chain, configuration, kernel, user land and target image),
which is run by GitHub on PRs and pushes:

`./build.sh`

Note: all the scripts must be executed within the top folder of
the ELKS repository as the current one (= TOPDIR).

## Full step by step procedure

1- Create a `cross` subfolder:

`mkdir cross`

2- Build the cross tool chain, mainly based on GCC-IA16. This
step is quite long, but only needs to be done once:

`tools/build.sh`

3- Set up your environment (PATH, TOPDIR and CROSSDIR):

`. ./env.sh` (note the '.' before the script, or use `source ./env.sh` for csh)

4- Configure the kernel, the user land and the target image format. This
creates the configuration file .config:

`make menuconfig`

5- Build the kernel, the user land and the target image:

`make all`

The target root folder is built in `target`, and depending on your
configuration, that folder is used to create either a floppy disk image
(fd360, fd720, fd1200, fd1440, fd2880), a flat 32MB hard disk image (without MBR),
or a ROM file image into the `image` folder. The image extension is '.img'
and will be in either ELKS (MINIX) or MSDOS (FAT) filesystem format.

6- Before writing that image on the real medium, you can test it first on QEMU:

`./qemu.sh`

7- You can then modify the configuration or the sources and repeat from the
step 4 after cleaning only the kernel, the user land and the image:

`make clean`

To clean the kernel build objects only, `make kclean` can be used.

8- One can also build ELKS distribution images for the entire suite of
supported floppy formats and hard disks (with and without MBRs) for both
MINIX and MSDOS FAT format. To create these images, use the following:

`cd image; make images`

## Build errors

* if you get a message similar to `Blocks required 1456, available 1440` then you need to enable Userland ---> Select Applications by Image Size in `make menuconfig`. 
