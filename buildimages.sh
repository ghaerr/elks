#
# Build all ELKS images

cleanup()
{
    make kclean
    rm -f bootblocks/*.o
    rm -f elkscmd/sys_utils/clock.o
    rm -f elkscmd/basic/*.o
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

# build IBM PC versions
build_ibm()
{
    cleanup
    cp ibmpc-1440.config .config
    make

    cd image
    make images
}

make clean
build_ibm
build_pc98

cp ibmpc-1440.config .config
cleanup
make
