# Makefile for cross tools

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

.PHONY: all prune

all::

# TOPDIR and CROSSDIR from environment

SRCDIR=$(TOPDIR)/tools

DISTDIR=$(CROSSDIR)/dist
BUILDDIR=$(CROSSDIR)/build

# BINUTILS for IA16

BINUTILS_VER=16d43d2ccde42385b55d2f714a77fa34a7d0d3b7
BINUTILS_DIST=binutils-ia16-$(BINUTILS_VER)

$(DISTDIR)/$(BINUTILS_DIST).zip:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/binutils-ia16-*.zip $(DISTDIR)/binutils-ia16-*.zip.tmp
	cd $(DISTDIR) && wget https://github.com/tkchia/binutils-ia16/archive/$(BINUTILS_VER).zip -O $(BINUTILS_DIST).zip.tmp
	cd $(DISTDIR) && mv $(BINUTILS_DIST).zip.tmp $(BINUTILS_DIST).zip

$(BUILDDIR)/.binutils.src: $(DISTDIR)/$(BINUTILS_DIST).zip
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(BINUTILS_DIST)
	cd $(BUILDDIR) && unzip -q $(DISTDIR)/$(BINUTILS_DIST).zip
	rm -rf $(BUILDDIR)/binutils-src
	cd $(BUILDDIR) && mv $(BINUTILS_DIST) binutils-src
	touch $(BUILDDIR)/.binutils.src

$(BUILDDIR)/.binutils.build: $(BUILDDIR)/.binutils.src
	cd $(BUILDDIR) && rm -rf binutils-build
	mkdir -p $(BUILDDIR)/binutils-build
	cd $(BUILDDIR)/binutils-build && ../binutils-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-nls
	$(MAKE) -C $(BUILDDIR)/binutils-build
	touch $(BUILDDIR)/.binutils.build

$(CROSSDIR)/.binutils.install: $(BUILDDIR)/.binutils.build
	$(MAKE) -C $(BUILDDIR)/binutils-build install
	touch $(CROSSDIR)/.binutils.install

all:: $(CROSSDIR)/.binutils.install

# GCC for IA16

GCC_VER=d3440aa96f9768b7268174278f9b80c1a85410bd
GCC_DIST=gcc-ia16-$(GCC_VER)

$(DISTDIR)/$(GCC_DIST).zip:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/gcc-ia16-*.zip $(DISTDIR)/gcc-ia16-*.zip.tmp
	cd $(DISTDIR) && wget https://github.com/tkchia/gcc-ia16/archive/$(GCC_VER).zip -O $(GCC_DIST).zip.tmp
	cd $(DISTDIR) && mv $(GCC_DIST).zip.tmp $(GCC_DIST).zip

$(BUILDDIR)/.gcc.src: $(DISTDIR)/$(GCC_DIST).zip
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(GCC_DIST)
	cd $(BUILDDIR) && unzip -q $(DISTDIR)/$(GCC_DIST).zip
	rm -rf $(BUILDDIR)/gcc-src
	cd $(BUILDDIR) && mv $(GCC_DIST) gcc-src
	cd $(BUILDDIR)/gcc-src && contrib/download_prerequisites
	touch $(BUILDDIR)/.gcc.src

$(BUILDDIR)/.gcc.build: $(BUILDDIR)/.gcc.src $(BUILDDIR)/.binutils.build
	cd $(BUILDDIR) && rm -rf gcc-build
	mkdir $(BUILDDIR)/gcc-build
	cd $(BUILDDIR)/gcc-build && ../gcc-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --without-headers --enable-languages=c --disable-libssp --without-isl
	$(MAKE) -C $(BUILDDIR)/gcc-build
	touch $(BUILDDIR)/.gcc.build

$(CROSSDIR)/.gcc.install: $(BUILDDIR)/.gcc.build
	$(MAKE) -C $(BUILDDIR)/gcc-build install
	touch $(CROSSDIR)/.gcc.install

all:: $(CROSSDIR)/.gcc.install

# EMU86

EMU86_VER=280167b8722ceb647ffd0405602a6fb5becd05ed
EMU86_DIST=emu86-$(EMU86_VER)

$(DISTDIR)/$(EMU86_DIST).zip:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/emu86-*.zip $(DISTDIR)/emu86-*.zip.tmp
	cd $(DISTDIR) && wget https://github.com/mfld-fr/emu86/archive/$(EMU86_VER).zip -O $(EMU86_DIST).zip.tmp
	cd $(DISTDIR) && mv $(EMU86_DIST).zip.tmp $(EMU86_DIST).zip

$(BUILDDIR)/.emu86.src: $(DISTDIR)/$(EMU86_DIST).zip
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(EMU86_DIST)
	cd $(BUILDDIR) && unzip -q $(DISTDIR)/$(EMU86_DIST).zip
	rm -rf $(BUILDDIR)/emu86
	cd $(BUILDDIR) && mv $(EMU86_DIST) emu86
	touch $(BUILDDIR)/.emu86.src

$(BUILDDIR)/.emu86.build: $(BUILDDIR)/.emu86.src
	$(MAKE) -C $(BUILDDIR)/emu86 all
	touch $(BUILDDIR)/.emu86.build

$(CROSSDIR)/.emu86.install: $(BUILDDIR)/.emu86.build
	install $(BUILDDIR)/emu86/emu86 $(CROSSDIR)/bin/
	install $(BUILDDIR)/emu86/pcat $(CROSSDIR)/bin/
	touch $(CROSSDIR)/.emu86.install

all:: $(CROSSDIR)/.emu86.install

# Tools pruning (to save disk space)

prune:
	-rm -rf $(DISTDIR)
	-rm -rf $(BUILDDIR)
