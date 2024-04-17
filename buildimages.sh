#
# This script is used to build all ELKS images outside Github CI

cleanup()
{
    make kclean
    rm -f bootblocks/*.o
    rm -f elkscmd/sys_utils/clock.o
    rm -f elkscmd/sys_utils/ps.o
    rm -f elkscmd/sys_utils/meminfo.o
    rm -f elkscmd/sys_utils/beep.o
    rm -f elkscmd/basic/*.o
    rm -f elkscmd/nano-X/*/*.o
}

# build PC-98 versions
build_pc98()
{
    cleanup
    cp pc98-1232.config .config
    make
    mv image/fd1232.img image/fd1232-pc98.img

    cleanup
    cp pc98-1440.config .config
    make
    mv image/fd1440.img image/fd1440-pc98.img
}

# build 8018X image
build_8018x()
{
    cleanup
    cp 8018x.config .config
    make
    mv image/romfs.bin image/romfs-8018x.bin
}

# build IBM PC versions
build_ibm()
{
    cleanup
    cp ibmpc-1440.config .config
    make

    cd image
    make images
    cd ..
}

make clean
build_ibm
build_pc98
build_8018x

cp ibmpc-1440.config .config
cleanup
make
