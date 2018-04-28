
ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

.PHONY: all

all::

# SRCDIR, DISTDIR, BUILDDIR and CROSSDIR must be set in environment

# DEV86 (including BIN86, BCC and C library)

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

.dev86.build: .dev86.src
	cd dev86 && make PREFIX=$(CROSSDIR) install
	touch .dev86.build

all:: .dev86.build

# ATT to AS86 conversion utility

ifdef CONFIG_COMPILER_2
all:: $(CROSSDIR)/bin/att2as86
endif

$(CROSSDIR)/bin/att2as86:
	make -C $(SRCDIR)/att2as86 all install

# BINUTILS for IA16

BINUTILS_VER=c045395ece435fcb3bf6225e9c4734b3ebcc874b
BINUTILS_DIST=binutils-ia16-$(BINUTILS_VER)

$(DISTDIR)/$(BINUTILS_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/crtc-demos/binutils-ia16/archive/$(BINUTILS_VER).zip -O $(BINUTILS_DIST).zip

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

ifdef CONFIG_COMPILER_2
all:: .binutils.build
endif

# GCC for IA16

GCC_VER=3c79fd7bc3d5a834491714fb0082a0f985e6ba83
GCC_DIST=gcc-ia16-$(GCC_VER)

$(DISTDIR)/$(GCC_DIST).zip:
	cd $(DISTDIR) && wget https://github.com/jbruchon/gcc-ia16/archive/$(GCC_VER).zip -O $(GCC_DIST).zip

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

ifdef CONFIG_COMPILER_2
all:: .gcc.build
endif
