.PHONY: all

# SRCDIR, DISTDIR, BUILDDIR and CROSSDIR must be set in environment

# ATT to AS86 conversion utility

all:: $(CROSSDIR)/bin/att2as86

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

all:: .binutils.build

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

all:: .gcc.build
