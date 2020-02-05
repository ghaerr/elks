![logo](https://github.com/elks-org/elks/blob/master/Documentation/img/ELKS-Logo.png)


![cross](https://github.com/elks-org/elks/workflows/cross/badge.svg)
![main](https://github.com/elks-org/elks/workflows/main/badge.svg)


What is this ?
--------------

This is a project to write a Linux-like OS for systems based on the Intel
IA16 architecture (16 bits processors: 8088, 8086, 80188, 80186, 80286,
Nec V20, V30 and compatibles).

Such systems are ancient computers (IBM-PC XT / AT and clones), or more
recent SBC / SoC / FPGA that reuse the huge hardware & software legacy
from that popular platform.


How to build ?
--------------

To build ELKS, you need a cross build tool chain, mainly based on the latest
GCC-IA16 (DEV86 including BCC was used for previous versions, but has been
dropped because it was obsolete and no more actively maintained).

Among the dependencies there are packages (for Debian and Ubuntu distros):
texinfo bison flex libgmp-dev libmpfr-dev libmpc-dev libncurses5-dev

A script is provided to automatically download and build that tool chain:

  'tools/build.sh'

Note: all the scripts must be executed within the top folder 'elks/' as the
current one.

A script is provided to automate the whole build process (configuration,
kernel, user land and target image) and make it easier for ELKS newbies:

  './build.sh'

If you want to clean everything up afterwards, run './build.sh clean'
and it will run 'make clean' in the build directories for you.

The general build procedure for ELKS is as follows:

* Set up your environment (PATH, TOPDIR and CROSSDIR):

  '. tools/env.sh' (note the '.' before the script)

* Build the cross chain in 'cross/' (see above)

* Configure the build chain, the kernel, the user land and the target image
  format:

  'make menuconfig'

* Build the kernel, the user land and the target image:

  'make all'

The target root folder is built in 'target/', and depending on your
configuration, that folder is packed as either a floppy disk image (fd1440,
fd1680, fd1200, fd720, fd360, without MBR), a hard-disk image (hd, with MBR),
or a file image (ROM, TAR), into the '/image' folder.

Before writting the image on the real device, you can test it first on QEMU
with './qemu.sh' (will configure QEMU as an ISA system).


More information
----------------

Questions? Problems? Patches? Open an issue in this project! You can also join
and email the 'Linux-8086' list at linux-8086@vger.kernel.org.

More information in the Documentation folder: Documentation/index.html
