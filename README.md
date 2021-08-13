# ELKS/pc98  
  
This is the fork repository for porting ELKS to PC-9801/PC-9821 architecture.  
(just started)  
  
PC-9801/PC-9821 are Japanese computers in 80's and 90's.  

Currently it is needed to adding definition of CONFIG_ARCH_PC98 manually.  
CONFIG_IMG_FD1232 is also needed to be defined to use 1232KiB, 1024Bytes per sector.  
  
Only CONFIG_IMG_FD1232 and CONFIG_IMG_FD1440 with FAT Filesystem are planned to support.  
  
Progress  
Boot : can call start_kernel  
Kernel : not yet  
driver : modifying bios FD read  
init : in progress  
command : not yet  


The following text is the original README of ELKS.  

--------------------------------------------------

![logo](https://github.com/jbruchon/elks/blob/master/Documentation/img/ELKS-Logo.png)


![cross](https://github.com/jbruchon/elks/workflows/cross/badge.svg)
![main](https://github.com/jbruchon/elks/workflows/main/badge.svg)


# What is this ?

This is a project providing a Linux-like OS for systems based on the Intel
IA16 architecture (16-bit processors: 8086, 8088, 80188, 80186, 80286,
NEC V20, V30 and compatibles).

Such systems are ancient computers (IBM-PC XT / AT and clones), or more
recent SBC / SoC / FPGA that reuse the huge hardware & software legacy
from that popular platform.

## Watch ELKS in action

- [ELKS, a 16-bit no-MMU Linux on Amstrad PC 2086](https://www.youtube.com/watch?v=eooviN1SdQ8) (thanks @pawoswm-arm)
- [Booting ELKS on an old 286 MB from 1,44MB floppy](https://www.youtube.com/watch?v=6rwlqmdebxk) (thanks @xrayer)
- [Epson PC Portable Q150A / Equity LT (Nec V30 8086 - 1989)](https://youtu.be/ZDffBj6zY-w?t=687) (thanks Alejandro)

## Screenshots

ELKS running on QEMU
![ss1](https://github.com/jbruchon/elks/blob/master/Screenshots/ELKS_0.4.0.png)

Olivetti M24 8086 CPU
![ss2](https://github.com/jbruchon/elks/blob/master/Screenshots/Olivetti_M24_8086_CPU.png)

ELKS Networking showing netstat and process list
![ss3](https://github.com/jbruchon/elks/blob/master/Screenshots/ELKS_Networking.png)

## Downloads

A full set of disk images are available for download, for you to try out ELKS: [Downloads](https://github.com/jbruchon/elks/releases).

## How to build ?

Full build instructions are [here](https://github.com/jbruchon/elks/blob/master/BUILD.md).

## Wiki

Help on how to use ELKS, as well as technical tutorials, are available on our [Wiki](https://github.com/jbruchon/elks/wiki).

## Documentation

More information is in the Documentation folder: [Index of ELKS Documentation](https://htmlpreview.github.io/?https://github.com/jbruchon/elks/blob/master/Documentation/index.html).

## More information

Questions? Problems? Patches? Open an issue in this project!

You can also join and email the 'Linux-8086' list at linux-8086@vger.kernel.org.
