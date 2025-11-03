#
# This script is used to build all ELKS images outside Github CI
# Usage: ./buildimages [fast|ibm]
#   The 'fast' option skips initial make clean, pc98-1440 and ibm-all images uncompressed
set -x
set -e

cleanup()
{
    make kclean
    rm -f bootblocks/*.o
    rm -f elkscmd/sys_utils/clock.o
    rm -f elkscmd/sys_utils/ps.o
    rm -f elkscmd/sys_utils/meminfo.o
    rm -f elkscmd/sys_utils/beep.o
    rm -f elkscmd/basic/*.o
}

# build PC-98 versions
build_pc98()
{
    cleanup
    cp pc98-1232.config .config
    make
    mv image/fd1232.img image/fd1232-pc98.img
}

build_pc98_fast()
{
    cleanup
    cp pc98-1232-nc.config .config
    make
    mv image/fd1232.img image/fd1232-pc98.img
}

build_pc98_1200()
{
    cleanup
    cp pc98-1200.config .config
    make
    mv image/fd1200.img image/fd1200-pc98.img
}

build_pc98_1440()
{
    cleanup
    cp pc98-1440.config .config
    make
    mv image/fd1440.img image/fd1440-pc98.img
}

# build 8018X rom image
build_rom_8018x()
{
    cleanup
    cp 8018x.config .config
    make
    cp -p elks/arch/i86/boot/Image image/rom-8018x.bin
    mv image/romfs.bin image/romfs-8018x.bin
}

# build NEC V25 rom image
build_rom_necv25()
{
    cleanup
    cp necv25.config .config
    make
    cp -p elks/arch/i86/boot/Image image/rom-necv25.bin
    mv image/romfs.bin image/romfs-necv25.bin
}

# build 8088 rom image
build_rom_8088()
{
    cleanup
    cp emu86-rom-full.config .config
    make
    cp -p elks/arch/i86/boot/Image image/rom-8088.bin
    mv image/romfs.bin image/romfs-8088.bin
}

# build Swan rom image
build_rom_swan()
{
    cleanup
    cp swan.config .config
    make
    mv image/rom.wsc image/rom-swan.wsc
}

# build IBM PC versions
build_ibm()
{
    cleanup
    cp ibmpc-1440.config .config
    make
}

build_ibm_fast()
{
    cleanup
    cp ibmpc-1440-nc.config .config
    make
}

build_ibm_all()
{
    cd image
    make images
    cd ..
}

# quick kernel-only and specific apps build for dosbox, emu86.sh and qemu.sh testing
if [ "$1" == "fast" ]; then
    build_pc98_fast
    build_rom_8088
    build_ibm_fast
    exit
fi

if [ "$1" == "ibm" ]; then
    build_ibm_fast
    exit
fi

# full (re)build including C library and all applications
make clean
build_pc98
build_pc98_1200
build_pc98_1440
build_rom_8018x
build_rom_necv25
build_rom_8088
build_rom_swan
build_ibm
build_ibm_all
