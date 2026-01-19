#!/usr/bin/env bash
#
# buildext.sh - build application repositories in extapps/, build OWC/C86 apps in elkscmd/
# This build script is called in main.yml by GitHub Continuous Integration
# 17 Jan 2026 Greg Haerr

set -e

doexit()
{
    E="$1"
    test -z $1 && E=0
    if [ $E -eq 0 ]
        then echo "Buildext.sh has completed successfully."
        else echo "Buildext.sh has terminated with error $E"
    fi
    exit $E
}

# Environment setup
add_path()
{
    if [[ ":$PATH:" != *":$1:"* ]]; then
        export PATH="$1:$PATH"
    fi
}

elks_environ()
{
    SCRIPTDIR="$(dirname "$0")"
    . "$SCRIPTDIR/env.sh"
    [ $? -ne 0 ] && doexit 1
    mkdir -p extapps
}

owc_environ()
{
    # OWC should be setup outside this script!
    #export WATCOM=/Users/greg/net/open-watcom-v2/rel
    #add_path "$WATCOM/binl"    # for Linux-32
    #add_path "$WATCOM/binl64"  # for Linux-64
    #add_path "$WATCOM/bino64"  # for macOS
    return 0
}

c86_environ()
{
    export C86=$TOPDIR/extapps/8086-toolchain
    add_path "$C86/host-bin"
    return 0
}

owc_libc()
{
    cd $TOPDIR
    make owclean
    make owlibc
}

c86_libc()
{
    c86_environ
    cd $TOPDIR
    make c86clean
    make c86libc
}

c86_toolchain()
{
    echo "Building 8086-toolchain..."
    cd $TOPDIR/extapps
    if [ ! -d 8086-toolchain ] ; then
        git clone https://github.com/ghaerr/8086-toolchain
    fi
    cd 8086-toolchain
    git pull
    make clean
    make host
    make elks
    c86_libc
    cd $C86/examples
    make
    echo "8086-toolchain build complete"
}

# build OWC apps in elkscmd/
owc_elkscmd()
{
    echo "Building OWC apps in elkscmd/"
    cd $TOPDIR
    make -C elkscmd owclean
    make -C elkscmd owc
    echo "OWC apps in elkscmd/ build complete"
}

# build C86 apps in elkscmd/
c86_elkscmd()
{
    echo "Building C86 apps in elkscmd/"
    cd $TOPDIR
    make -C elkscmd c86clean
    make -C elkscmd c86
    echo "C86 apps in elkscmd/ build complete"
}

# external repositories
microwindows()
{
    echo "Building Nano-X..."
    cd $TOPDIR/extapps
    if [ ! -d microwindows ] ; then
        git clone https://github.com/ghaerr/microwindows.git
    fi
    cd microwindows/src
    git pull
    make -f Makefile.elks clean
    make -f Makefile.elks
    echo "Nano-X build complete"
}

dflat()
{
    echo "Building D-Flat..."
    cd $TOPDIR/extapps
    if [ ! -d dflat ] ; then
        git clone https://github.com/ghaerr/dflat.git
    fi
    cd dflat
    git pull
    make -f Makefile.elks clean
    make -f Makefile.elks
    echo "D-Flat build complete"
}

doom()
{
    echo "Building Doom..."
    cd $TOPDIR/extapps
    if [ ! -d elksdoom ] ; then
        git clone https://github.com/FrenkelS/elksdoom
    fi
    cd elksdoom
    git pull
    rm -f *.obj
    INCLUDE="$TOPDIR/libc/include:$TOPDIR/include:$TOPDIR/elks/include:$WATCOM/h" \
    LIBC=$TOPDIR/libc/libcm.lib \
    ./compelks.sh
    echo "Doom build complete"
}

ngircd_elks()
{
    echo "Building ngircd-elks..."
    cd $TOPDIR/extapps
    if [ ! -d ngircd-elks ] ; then
        git clone https://github.com/parabyte/ngircd-elks
    fi
    cd ngircd-elks
    git pull
    NGIRCD_DIR=$TOPDIR/extapps/ngircd-elks make -e -f Makefile.owc clean
    NGIRCD_DIR=$TOPDIR/extapps/ngircd-elks make -e -f Makefile.owc
    echo "ngircd-elks build complete"
}

elkirc()
{
    echo "Building elkirc..."
    cd $TOPDIR/extapps
    if [ ! -d elkirc ] ; then
        git clone https://github.com/ghaerr/elkirc
        git checkout elks
    fi
    cd elkirc
    git pull
    make clean
    make ELKS=1
    echo "elkirc build complete"
}

# build all extapps repos
make_all()
{
    microwindows
    dflat
    elkirc
    if [ -n "$WATCOM" ] ; then
        owc_libc
        owc_elkscmd
        c86_toolchain
        c86_elkscmd
        doom
        ngircd_elks
    fi
}

# script starts here

if [ "$1" = "" ] ; then
echo "Usage: $0 [all | <repo_name>]"
doexit 1
fi

elks_environ
#owc_environ
#c86_environ

if [ "$1" = "all" ] ; then
    make_all
else
    for repo in $@ ; do
        $repo
    done
fi

doexit 0
