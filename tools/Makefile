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

BINUTILS_VER=69e12ea2c125ff830580abbe4699d35ba002148b
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
	cd $(BUILDDIR)/binutils-build && ../binutils-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --enable-ld=default --enable-gold=yes --enable-targets=ia16-elf --enable-x86-hpa-segelf=yes --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-nls
	$(MAKE) -C $(BUILDDIR)/binutils-build $(PARALLEL)
	touch $(BUILDDIR)/.binutils.build

$(CROSSDIR)/.binutils.install: $(BUILDDIR)/.binutils.build
	$(MAKE) -C $(BUILDDIR)/binutils-build install
	touch $(CROSSDIR)/.binutils.install

all:: $(CROSSDIR)/.binutils.install

# GCC prerequisites: GNU MP, MPFR, and MPC libraries

GMP_VER=6.1.2
GMP_DIST=gmp-$(GMP_VER)
MPFR_VER=3.1.5
MPFR_DIST=mpfr-$(MPFR_VER)
MPC_VER=1.0.3
MPC_DIST=mpc-$(MPC_VER)

$(DISTDIR)/$(GMP_DIST).tar.bz2:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/gmp-*.tar.bz2 $(DISTDIR)/gmp-*.tar.bz2.tmp
	wget -c https://ftp.gnu.org/gnu/gmp/$(GMP_DIST).tar.bz2 \
		-O $@.tmp || \
	wget -c https://gmplib.org/download/gmp/$(GMP_DIST).tar.bz2 \
		-O $@.tmp
	# Try to force GCC to be rebuilt whenever GNU MP needs to be
	# re-downloaded.  (I also use the same hack for mpfr and mpc, below.)
	#	-- tkchia 20201018
	touch $@.tmp
	mv $@.tmp $@

$(DISTDIR)/$(MPFR_DIST).tar.bz2:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/mpfr-*.tar.bz2 $(DISTDIR)/mpfr-*.tar.bz2.tmp
	wget -c https://ftp.gnu.org/gnu/mpfr/$(MPFR_DIST).tar.bz2 \
		-O $@.tmp
	touch $@.tmp
	mv $@.tmp $@

$(DISTDIR)/$(MPC_DIST).tar.gz:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/mpc-*.tar.gz $(DISTDIR)/mpc-*.tar.gz.tmp
	wget -c https://ftp.gnu.org/gnu/mpc/$(MPC_DIST).tar.gz -O $@.tmp
	touch $@.tmp
	mv $@.tmp $@

# GCC for IA16

GCC_VER=ca893320926dc93552390b892a202e9373d040c0
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

$(BUILDDIR)/.gcc.src: $(DISTDIR)/$(GCC_DIST).tar.gz \
		      $(DISTDIR)/$(GMP_DIST).tar.bz2 \
		      $(DISTDIR)/$(MPFR_DIST).tar.bz2 \
		      $(DISTDIR)/$(MPC_DIST).tar.gz
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(GCC_DIST)
	cd $(BUILDDIR) && tar -xzf $(DISTDIR)/$(GCC_DIST).tar.gz
	rm -rf $(BUILDDIR)/gcc-src
	cd $(BUILDDIR) && mv $(GCC_DIST) gcc-src
	cd $(BUILDDIR)/gcc-src && tar -xjf $(DISTDIR)/$(GMP_DIST).tar.bz2
	cd $(BUILDDIR)/gcc-src && ln -s $(GMP_DIST) gmp
	cd $(BUILDDIR)/gcc-src && tar -xjf $(DISTDIR)/$(MPFR_DIST).tar.bz2
	cd $(BUILDDIR)/gcc-src && ln -s $(MPFR_DIST) mpfr
	cd $(BUILDDIR)/gcc-src && tar -xzf $(DISTDIR)/$(MPC_DIST).tar.gz
	cd $(BUILDDIR)/gcc-src && ln -s $(MPC_DIST) mpc
	touch $(BUILDDIR)/.gcc.src

$(BUILDDIR)/.gcc.build: $(BUILDDIR)/.gcc.src $(BUILDDIR)/.binutils.build
	cd $(BUILDDIR) && rm -rf gcc-build
	mkdir $(BUILDDIR)/gcc-build
	cd $(BUILDDIR)/gcc-build && ../gcc-src/configure --target=ia16-elf --prefix="$(CROSSDIR)" --without-headers --enable-languages=c --disable-libssp --without-isl
	# If there are any obsolete multilib directories (which are no longer
	# used) in the installation directory, remove them, so that they do
	# not clutter up the installation directory.  This is a bit of a hack.
	#	-- tkchia 20201128
	rm -rf $(CROSSDIR)/lib/gcc/ia16-elf/6.3.0/size \
	       $(CROSSDIR)/lib/gcc/ia16-elf/6.3.0/rtd/size \
	       $(CROSSDIR)/lib/gcc/ia16-elf/6.3.0/regparmcall/size \
	       $(CROSSDIR)/lib/gcc/ia16-elf/6.3.0/regparmcall/any_186/size
	# Now build.
	$(MAKE) -C $(BUILDDIR)/gcc-build $(PARALLEL)
	touch $(BUILDDIR)/.gcc.build

$(CROSSDIR)/.gcc.install: $(BUILDDIR)/.gcc.build
	$(MAKE) -C $(BUILDDIR)/gcc-build install
	touch $(CROSSDIR)/.gcc.install

all:: $(CROSSDIR)/.gcc.install

# EMU86

EMU86_VER=16dfcf0d68dccf9211a575c2a6ff90d5a192dd61
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
#	cp $(BUILDDIR)/emu86/config-elks.mk $(BUILDDIR)/emu86/config.mk
	$(MAKE) -C $(BUILDDIR)/emu86 all
	touch $(BUILDDIR)/.emu86.build

$(CROSSDIR)/.emu86.install: $(BUILDDIR)/.emu86.build
	install $(BUILDDIR)/emu86/emu86 $(CROSSDIR)/bin/
	install $(BUILDDIR)/emu86/pcat $(CROSSDIR)/bin/
	touch $(CROSSDIR)/.emu86.install

all:: $(CROSSDIR)/.emu86.install

# wf-mednafen

WF_MEDNAFEN_VER=6203a47cce4a0a44a5f9f58618bbfc50e0ab45e7
WF_MEDNAFEN_DIST=wf-mednafen-$(WF_MEDNAFEN_VER)

$(DISTDIR)/$(WF_MEDNAFEN_DIST).tar.gz:
	mkdir -p $(DISTDIR)
	rm -rf $(DISTDIR)/wf-mednafen-*.tar.gz \
	       $(DISTDIR)/wf-mednafen-*.tmp \
	       $(DISTDIR)/wf-mednafen-*.zip
	set -e; \
	cd $(DISTDIR); \
	until gunzip -t $(WF_MEDNAFEN_DIST).tar.gz.tmp 2>/dev/null; do \
		wget -c https://github.com/WonderfulToolchain/wf-mednafen/archive/$(WF_MEDNAFEN_VER).tar.gz \
		     -O $(WF_MEDNAFEN_DIST).tar.gz.tmp; \
	done
	cd $(DISTDIR) && mv $(WF_MEDNAFEN_DIST).tar.gz.tmp $(WF_MEDNAFEN_DIST).tar.gz

$(BUILDDIR)/.wf-mednafen.src: $(DISTDIR)/$(WF_MEDNAFEN_DIST).tar.gz
	mkdir -p $(BUILDDIR)
	rm -rf $(BUILDDIR)/$(WF_MEDNAFEN_DIST)
	cd $(BUILDDIR) && tar -xzf $(DISTDIR)/$(WF_MEDNAFEN_DIST).tar.gz
	rm -rf $(BUILDDIR)/wf-mednafen
	cd $(BUILDDIR) && mv $(WF_MEDNAFEN_DIST) wf-mednafen
	touch $(BUILDDIR)/.wf-mednafen.src

$(BUILDDIR)/.wf-mednafen.build: $(BUILDDIR)/.wf-mednafen.src
	cd $(BUILDDIR)/wf-mednafen/mednafen && ./autogen.sh
	mkdir -p $(BUILDDIR)/wf-mednafen-build
	# Disable most non-wswan emulator cores to save disk space and build time.
	cd $(BUILDDIR)/wf-mednafen-build && ../wf-mednafen/mednafen/configure --disable-apple2 --disable-gb \
		--disable-gba --disable-lynx --disable-md --disable-nes --disable-ngp --disable-psx \
		--disable-sasplay --disable-sms --disable-snes --disable-snes-faust --disable-ssfplay \
		--disable-ss --disable-vb --disable-nls --disable-fancy-scalers
	$(MAKE) -C $(BUILDDIR)/wf-mednafen-build $(PARALLEL)
	touch $(BUILDDIR)/.wf-mednafen.build

$(CROSSDIR)/.wf-mednafen.install: $(BUILDDIR)/.wf-mednafen.build
	install $(BUILDDIR)/wf-mednafen-build/src/mednafen $(CROSSDIR)/bin/wf-mednafen
	touch $(CROSSDIR)/.wf-mednafen.install

swan:: $(CROSSDIR)/.wf-mednafen.install

# Tools pruning (to save disk space)

prune:
	-rm -rf $(DISTDIR)
	-rm -rf $(BUILDDIR)
