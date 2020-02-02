# Makefile for cross tools

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

.PHONY: all

all::

# SRCDIR, DISTDIR, BUILDDIR and CROSSDIR must be set in environment

# BINUTILS for IA16

BINUTILS_VER=3fc69e3b2e9864a7357027a9c37c37fa6c55685d
BINUTILS_DIST=binutils-ia16-$(BINUTILS_VER)

$(DISTDIR)/$(BINUTILS_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/tkchia/binutils-ia16/archive/$(BINUTILS_VER).zip -O $(BINUTILS_DIST).zip

.binutils.src: $(DISTDIR)/$(BINUTILS_DIST).zip
	-rm -rf $(BINUTILS_DIST)
	unzip -q $(DISTDIR)/$(BINUTILS_DIST).zip
	-rm -rf binutils-src
	mv $(BINUTILS_DIST) binutils-src
	touch .binutils.src

.binutils.build: .binutils.src
	-rm -rf binutils-build
	mkdir binutils-build
	cd binutils-build && ../binutils-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-nls
	$(MAKE) -C binutils-build
	$(MAKE) -C binutils-build install
	touch .binutils.build

all:: .binutils.build

# GCC for IA16

GCC_VER=2dd73a5500dce4be8f0f7d4bab02f5ed6bf849b0
GCC_DIST=gcc-ia16-$(GCC_VER)

$(DISTDIR)/$(GCC_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/tkchia/gcc-ia16/archive/$(GCC_VER).zip -O $(GCC_DIST).zip

.gcc.src: $(DISTDIR)/$(GCC_DIST).zip
	-rm -rf $(GCC_DIST)
	unzip -q $(DISTDIR)/$(GCC_DIST).zip
	-rm -rf gcc-src
	mv $(GCC_DIST) gcc-src
	touch .gcc.src
	cd gcc-src && contrib/download_prerequisites

.gcc.build: .gcc.src .binutils.build
	-rm -rf gcc-build
	mkdir gcc-build
	cd gcc-build && ../gcc-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --without-headers --enable-languages=c --disable-libssp --without-isl
	$(MAKE) -C gcc-build
	$(MAKE) -C gcc-build install
	touch .gcc.build

all:: .gcc.build

# EMU86

EMU86_VER=5ea2e76a30c75223c484c3b1bd2f67646e3cfa94
EMU86_DIST=emu86-$(EMU86_VER)

$(DISTDIR)/$(EMU86_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/mfld-fr/emu86/archive/$(EMU86_VER).zip -O $(EMU86_DIST).zip

.emu86.src: $(DISTDIR)/$(EMU86_DIST).zip
	-rm -rf $(EMU86_DIST)
	unzip -q $(DISTDIR)/$(EMU86_DIST).zip
	-rm -rf emu86
	mv $(EMU86_DIST) emu86
	touch .emu86.src

.emu86.build: .emu86.src
	$(MAKE) -C emu86 all
	install emu86/emu86 $(CROSSDIR)/bin/
	install emu86/pcat $(CROSSDIR)/bin/
	touch .emu86.build

all:: .emu86.build
