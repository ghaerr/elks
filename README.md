![logo](https://github.com/elks-org/elks/blob/master/Documentation/img/ELKS-Logo.png)


![cross](https://github.com/elks-org/elks/workflows/cross/badge.svg)
![main](https://github.com/elks-org/elks/workflows/main/badge.svg)


# What is this ?

This is a project to write a Linux-like OS for systems based on the Intel
IA16 architecture (16 bits processors: 8088, 8086, 80188, 80186, 80286,
Nec V20, V30 and compatibles).

Such systems are ancient computers (IBM-PC XT / AT and clones), or more
recent SBC / SoC / FPGA that reuse the huge hardware & software legacy
from that popular platform.

Watch ELKS in action (thanks @xrayer): https://www.youtube.com/watch?v=6rwlqmdebxk

# How to build ?

## Prerequisites

To build ELKS, you need a GNU development environment, including:
- flex
- bison
- texinfo
- libncurses5-dev

## Quickstart

A script is provided to automate the whole build process
(cross toool chain, configuration, kernel, user land and target image),
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
dropped because it was obsolete and no more maintained):

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

The target root folder is built in `target`', and depending on your
configuration, that folder is packed as either a floppy disk image
(fd360, fd720, fd1440), a hard disk image (hd, without MBR),
or a file image (ROM, TAR), into the `image` folder.

6- Before writting that image on the real medium,
you can test it first on QEMU:

`./qemu.sh`

7- You can then modify the configuration or the sources and repeat from the
step 4 after cleaning only the kernel, the user land and the image:

`make clean`


# More information

Questions? Problems? Patches? Open an issue in this project!

You can also join and email the 'Linux-8086' list at linux-8086@vger.kernel.org.

More information in the Documentation folder: [Index of ELKS Documentation](https://htmlpreview.github.io/?https://github.com/jbruchon/elks/blob/master/Documentation/index.html)

