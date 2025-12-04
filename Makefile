
ifndef TOPDIR
$(error TOPDIR is not defined; did you mean to run './build.sh' instead?)
endif

include $(TOPDIR)/Make.defs

.PHONY: all clean libc kconfig defconfig config menuconfig image images \
    kimage kernel kclean owc c86

all: .config include/autoconf.h
	$(MAKE) -C libc all
	$(MAKE) -C libc -j1 DESTDIR='$(TOPDIR)/cross' install
	$(MAKE) -C elks -j1 all
	$(MAKE) -C bootblocks -j1 all
	$(MAKE) -C elkscmd -j1 all
	$(MAKE) -C image -j1 all
ifeq ($(shell uname), Linux)
	$(MAKE) -C elksemu PREFIX='$(TOPDIR)/cross' elksemu
endif

image:
	$(MAKE) -C image -j1

images:
	$(MAKE) -C image -j1 images

kimage: kernel image

kernel:
	$(MAKE) -C elks -j1

kclean:
	$(MAKE) -C elks -j1 kclean

clean:
	$(MAKE) -C libc -j1 clean
	$(MAKE) -C libc -j1 DESTDIR='$(TOPDIR)/cross' uninstall
	$(MAKE) -C elks -j1 clean
	$(MAKE) -C bootblocks -j1 clean
	$(MAKE) -C elkscmd -j1 clean
	$(MAKE) -C image -j1 clean
ifeq ($(shell uname), Linux)
	$(MAKE) -C elksemu -j1 clean
endif
	@echo
	@if [ ! -f .config ]; then \
	    echo ' * This system is not configured. You need to run' ;\
	    echo ' * `make config` or `make menuconfig` to configure it.' ;\
	    echo ;\
	fi

libc:
	$(MAKE) -C libc -j1 DESTDIR='$(TOPDIR)/cross' uninstall
	$(MAKE) -C libc all
	$(MAKE) -C libc -j1 DESTDIR='$(TOPDIR)/cross' install

owclean:
	$(MAKE) -C libc -j1 -f watcom.mk clean
	$(MAKE) -C elkscmd -j1 owclean

owlibc:
	$(MAKE) -C libc -f watcom.mk MODEL=c
	$(MAKE) -C libc -f watcom.mk MODEL=s
	$(MAKE) -C libc -f watcom.mk MODEL=m
	$(MAKE) -C libc -f watcom.mk MODEL=l

owc: owlibc
	$(MAKE) -C elkscmd -j1 owc

c86clean:
	$(MAKE) -C libc -f c86.mk -j1 clean
	$(MAKE) -C elkscmd -j1 c86clean

c86libc:
	$(MAKE) -C libc -f c86.mk -j1

c86: c86libc
	$(MAKE) -C elkscmd -j1 c86

elks/arch/i86/drivers/char/KeyMaps/config.in:
	$(MAKE) -C elks/arch/i86/drivers/char/KeyMaps -j1 config.in

kconfig:
	$(MAKE) -C config -j1 all

defconfig:
	$(RM) .config
	@yes '' | ${MAKE} config

include/autoconf.h: .config
	@yes '' | config/Configure -D config.in

config: elks/arch/i86/drivers/char/KeyMaps/config.in kconfig
	config/Configure config.in

menuconfig: elks/arch/i86/drivers/char/KeyMaps/config.in kconfig
	config/Menuconfig config.in

.PHONY: tags
tags:
	ctags -R --exclude=cross --exclude=elkscmd .
