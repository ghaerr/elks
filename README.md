Now it is started merging to the original ELKS repository.  
New features will be added there.  
https://github.com/jbruchon/elks  

# ELKS/pc98  
  
This is the fork repository for porting ELKS to PC-9801/PC-9821 architecture.  
  
PC-9801/PC-9821 are Japanese computers in 80's and 90's.  
  
CONFIG_ARCH_PC98 is needed to be defined.  
CONFIG_IMG_FD1232 is also needed to be defined to use 1232KiB, 1024Bytes per sector.  
  
Only CONFIG_IMG_FD1232 and CONFIG_IMG_FD1440 with FAT Filesystem are planned to support.  
  
Progress (with Neko Project 21/W emulator and PC-9801RX21, PC-9801UV21)  
Boot : can call start_kernel  
Kernel : modifying irq, timer  
driver : modifying bios FD read, console-headless, console-direct, kbd-poll  
init : can login with root  
command : cat,clear,clock,date,ed,kilo,ls,more,ps,pwd,vi can be used  

## How to build (for now)  
./build.sh to make the environment.  
Overwrite the .config with the content of pc98.config.  
make menuconfig.  
(Exit and save configuration to create include/autoconf.h)  
make clean  
make  
FD image will be created under image/ 
  
## Screenshots

ELKS/pc98 running on PC-9801UV21  
<img src=Screenshots/PC-9801UV21.png width="400pix"> 
  
## Downloads  
Test images will be available for download.  
[Downloads](https://github.com/tyama501/elks/releases)  
  
## Reference  
INT 1Bh (DISK BIOS)  
http://radioc.web.fc2.com/column/pc98bas/bios/disk.htm  
 
PIT 8254 (8253 for PC-9801)  
http://oswiki.osask.jp/?%28PIT%298254  

INT 18h (Keyboard BIOS)  
https://web.archive.org/web/20051120083002/http://www2.muroran-it.ac.jp/circle/mpc/program/pc98dos/keyboard.html  

Text V-RAM  
http://software.aufheben.info/contents.html?contents_key=textvram  

The following text is the original README of ELKS.  

--------------------------------------------------

![logo](https://github.com/jbruchon/elks/blob/master/Documentation/img/ELKS-Logo.png)


![cross](https://github.com/jbruchon/elks/workflows/cross/badge.svg)
![main](https://github.com/jbruchon/elks/workflows/main/badge.svg)


# What is ELKS?

ELKS is a project providing a Linux-like OS for systems based on the Intel
IA16 architecture (16-bit processors: 8086, 8088, 80188, 80186, 80286,
NEC V20, V30 and compatibles). Such systems are ancient computers (IBM-PC
XT / AT and clones) as well as more recent SBCs, SoCs, and FPGAs.

## Watch ELKS in action

- [ELKS, a 16-bit no-MMU Linux on Amstrad PC 2086](https://www.youtube.com/watch?v=eooviN1SdQ8) (thanks @pawoswm-arm)
- [Booting ELKS on an old 286 MB from 1,44MB floppy](https://www.youtube.com/watch?v=6rwlqmdebxk) (thanks @xrayer)
- [Epson PC Portable Q150A / Equity LT (Nec V30 8086 - 1989)](https://youtu.be/ZDffBj6zY-w?t=687) (thanks Alejandro)

## Screenshots

ELKS running on QEMU
![ss1](https://github.com/jbruchon/elks/blob/master/Screenshots/ELKS_0.5.0.png)

Olivetti M24 8086 CPU
![ss2](https://github.com/jbruchon/elks/blob/master/Screenshots/Olivetti_M24_8086_CPU.png)

ELKS Networking showing netstat and process list
![ss3](https://github.com/jbruchon/elks/blob/master/Screenshots/ELKS_Networking.png)

Running ELKS Basic on PC-9801UV21 (NEC V30 CPU)
![ss4](https://github.com/jbruchon/elks/blob/master/Screenshots/PC-9801UV21_V30_CPU.png)

## Downloads

A full set of disk images are available for download, for you to try out ELKS: [Downloads](https://github.com/jbruchon/elks/releases).

## How to build

Full build instructions are [here](https://github.com/jbruchon/elks/blob/master/BUILD.md).

## Wiki

Help on how to use ELKS, as well as technical tutorials, are available on our [Wiki](https://github.com/jbruchon/elks/wiki).

## Documentation

More information is in the Documentation folder: [Index of ELKS Documentation](https://htmlpreview.github.io/?https://github.com/jbruchon/elks/blob/master/Documentation/index.html).

## Resources

Other projects and resources interesting to ELKS and our programming community:

- [Size Optimization Tricks](https://justine.lol/sizetricks/) A great article from Justine Tunney's blog showing how big things can be done without bloat.
- [Blinkenlights](https://justine.lol/blinkenlights/) A visual debugger shows 8086 instruction execution starting from a PC boot sector.
- [gcc-ia16](https://github.com/tkchia/gcc-ia16) TK Chia's gcc compiler targeted for 8086, maintained and used for the ELKS kernel and all its applications.

## More information

Questions? Problems? Patches? Open an issue on the ELKS GitHub project!
