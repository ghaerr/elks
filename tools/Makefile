# Makefile for cross tools

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

.PHONY: all

all::

# SRCDIR, DISTDIR, BUILDDIR and CROSSDIR must be set in environment

# DEV86 (including BIN86, BCC and C library)
# Only sources now as not used any more for ELKS build
# but still for test with EMU86

DEV86_VER=dfb9b428df88dd3c2256f978351fa9959caf5c07
DEV86_DIST=dev86-$(DEV86_VER)

$(DISTDIR)/$(DEV86_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/jbruchon/dev86/archive/$(DEV86_VER).zip -O $(DEV86_DIST).zip

.dev86.src: $(DISTDIR)/$(DEV86_DIST).zip
	-rm -rf $(DEV86_DIST)
	unzip -q $(DISTDIR)/$(DEV86_DIST).zip
	-rm -rf dev86
	mv $(DEV86_DIST) dev86
	touch .dev86.src

all:: .dev86.src

# BINUTILS for IA16

BINUTILS_VER=184ba458e1ab8e46d09c3dee48790e93141793f2
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
	make -C binutils-build
	make -C binutils-build install
	touch .binutils.build

all:: .binutils.build

# GCC for IA16

GCC_VER=eb57e7ea98ebf30ff455fbde1af4c7279cc030bc
GCC_DIST=gcc-ia16-$(GCC_VER)

$(DISTDIR)/$(GCC_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/tkchia/gcc-ia16/archive/$(GCC_VER).zip -O $(GCC_DIST).zip

.gcc.src: $(DISTDIR)/$(GCC_DIST).zip
	-rm -rf $(GCC_DIST)
	unzip -q $(DISTDIR)/$(GCC_DIST).zip
	-rm -rf gcc-src
	mv $(GCC_DIST) gcc-src
	touch .gcc.src

.gcc.build: .gcc.src .binutils.build
	-rm -rf gcc-build
	mkdir gcc-build
	cd gcc-build && ../gcc-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --without-headers --enable-languages=c --disable-libssp --without-isl
	make -C gcc-build
	make -C gcc-build install
	touch .gcc.build

all:: .gcc.build
