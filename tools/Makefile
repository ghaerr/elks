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

# Building BINUTILS and GCC in parallel

# PARALLEL = -j3

# BINUTILS for IA16

BINUTILS_VER=41db51b04f06a66e2817a205ede9aeda7996263a
BINUTILS_DIST=binutils-ia16-$(BINUTILS_VER)

$(DISTDIR)/$(BINUTILS_DIST).tar.gz:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/binutils-ia16-*.tar.gz \
	       $(DISTDIR)/binutils-ia16-*.tmp \
	       $(DISTDIR)/binutils-ia16-*.zip
	set -e; \
	cd $(DISTDIR); \
	until gunzip -t $(BINUTILS_DIST).tar.gz.tmp 2>/dev/null; do \
		wget -c https://github.com/tkchia/binutils-ia16/archive/$(BINUTILS_VER).tar.gz \
		     -O $(BINUTILS_DIST).tar.gz.tmp; \
	done
	cd $(DISTDIR) && mv $(BINUTILS_DIST).tar.gz.tmp $(BINUTILS_DIST).tar.gz

$(BUILDDIR)/.binutils.src: $(DISTDIR)/$(BINUTILS_DIST).tar.gz
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(BINUTILS_DIST)
	cd $(BUILDDIR) && tar -xzf $(DISTDIR)/$(BINUTILS_DIST).tar.gz
	rm -rf $(BUILDDIR)/binutils-src
	cd $(BUILDDIR) && mv $(BINUTILS_DIST) binutils-src
	touch $(BUILDDIR)/.binutils.src

$(BUILDDIR)/.binutils.build: $(BUILDDIR)/.binutils.src
	cd $(BUILDDIR) && rm -rf binutils-build
	mkdir -p $(BUILDDIR)/binutils-build
	cd $(BUILDDIR)/binutils-build && ../binutils-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --enable-ld=default --enable-gold=yes --enable-targets=ia16-elf --enable-x86-hpa-segelf=yes --enable-hpa-segelf=yes --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-nls
	$(MAKE) -C $(BUILDDIR)/binutils-build $(PARALLEL)
	touch $(BUILDDIR)/.binutils.build

$(CROSSDIR)/.binutils.install: $(BUILDDIR)/.binutils.build
	$(MAKE) -C $(BUILDDIR)/binutils-build install
	touch $(CROSSDIR)/.binutils.install

all:: $(CROSSDIR)/.binutils.install

# GCC for IA16

GCC_VER=45605f97ac4d313f8783e1ba9924d0aec0b1d6e8
GCC_DIST=gcc-ia16-$(GCC_VER)

$(DISTDIR)/$(GCC_DIST).tar.gz:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/gcc-ia16-*.tar.gz \
	       $(DISTDIR)/gcc-ia16-*.tmp \
	       $(DISTDIR)/gcc-ia16-*.zip
	set -e; \
	cd $(DISTDIR); \
	until gunzip -t $(GCC_DIST).tar.gz.tmp 2>/dev/null; do \
		wget -c https://github.com/tkchia/gcc-ia16/archive/$(GCC_VER).tar.gz \
		     -O $(GCC_DIST).tar.gz.tmp; \
	done
	cd $(DISTDIR) && mv $(GCC_DIST).tar.gz.tmp $(GCC_DIST).tar.gz

$(BUILDDIR)/.gcc.src: $(DISTDIR)/$(GCC_DIST).tar.gz
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(GCC_DIST)
	cd $(BUILDDIR) && tar -xzf $(DISTDIR)/$(GCC_DIST).tar.gz
	rm -rf $(BUILDDIR)/gcc-src
	cd $(BUILDDIR) && mv $(GCC_DIST) gcc-src
	cd $(BUILDDIR)/gcc-src && contrib/download_prerequisites
	touch $(BUILDDIR)/.gcc.src

$(BUILDDIR)/.gcc.build: $(BUILDDIR)/.gcc.src $(BUILDDIR)/.binutils.build
	cd $(BUILDDIR) && rm -rf gcc-build
	mkdir $(BUILDDIR)/gcc-build
	cd $(BUILDDIR)/gcc-build && ../gcc-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --without-headers --enable-languages=c --disable-libssp --without-isl
	$(MAKE) -C $(BUILDDIR)/gcc-build $(PARALLEL)
	touch $(BUILDDIR)/.gcc.build

$(CROSSDIR)/.gcc.install: $(BUILDDIR)/.gcc.build
	$(MAKE) -C $(BUILDDIR)/gcc-build install
	touch $(CROSSDIR)/.gcc.install

all:: $(CROSSDIR)/.gcc.install

# EMU86

EMU86_VER=280167b8722ceb647ffd0405602a6fb5becd05ed
EMU86_DIST=emu86-$(EMU86_VER)

$(DISTDIR)/$(EMU86_DIST).tar.gz:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/emu86-*.tar.gz \
	       $(DISTDIR)/emu86-*.tmp \
	       $(DISTDIR)/emu86-*.zip
	set -e; \
	cd $(DISTDIR); \
	until gunzip -t $(EMU86_DIST).tar.gz.tmp 2>/dev/null; do \
		wget -c https://github.com/mfld-fr/emu86/archive/$(EMU86_VER).tar.gz \
		     -O $(EMU86_DIST).tar.gz.tmp; \
	done
	cd $(DISTDIR) && mv $(EMU86_DIST).tar.gz.tmp $(EMU86_DIST).tar.gz

$(BUILDDIR)/.emu86.src: $(DISTDIR)/$(EMU86_DIST).tar.gz
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(EMU86_DIST)
	cd $(BUILDDIR) && tar -xzf $(DISTDIR)/$(EMU86_DIST).tar.gz
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
