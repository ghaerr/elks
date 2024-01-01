
ifndef TOPDIR
$(error TOPDIR is not defined; did you mean to run './build.sh' instead?)
endif

include $(TOPDIR)/Make.defs

.PHONY: all clean libc kconfig defconfig config menuconfig

all: .config include/autoconf.h
	$(MAKE) -C libc all
	$(MAKE) -C libc DESTDIR='$(TOPDIR)/cross' install
	$(MAKE) -C elks all
	$(MAKE) -C bootblocks all
	$(MAKE) -C elkscmd all
	$(MAKE) -C image all
ifeq ($(shell uname), Linux)
	$(MAKE) -C elksemu PREFIX='$(TOPDIR)/cross' elksemu
endif

kclean:
	$(MAKE) -C elks kclean

clean:
	$(MAKE) -C libc clean
	$(MAKE) -C libc DESTDIR='$(TOPDIR)/cross' uninstall
	$(MAKE) -C elks clean
	$(MAKE) -C bootblocks clean
	$(MAKE) -C elkscmd clean
	$(MAKE) -C image clean
ifeq ($(shell uname), Linux)
	$(MAKE) -C elksemu clean
endif
	@echo
	@if [ ! -f .config ]; then \
	    echo ' * This system is not configured. You need to run' ;\
	    echo ' * `make config` or `make menuconfig` to configure it.' ;\
	    echo ;\
	fi

libc:
	$(MAKE) -C libc DESTDIR='$(TOPDIR)/cross' uninstall
	$(MAKE) -C libc all
	$(MAKE) -C libc DESTDIR='$(TOPDIR)/cross' install

elks/arch/i86/drivers/char/KeyMaps/config.in:
	$(MAKE) -C elks/arch/i86/drivers/char/KeyMaps config.in

kconfig:
	$(MAKE) -C config all

defconfig:
	$(RM) .config
	@yes '' | ${MAKE} config

include/autoconf.h: .config
	@yes '' | config/Configure -D config.in

config: elks/arch/i86/drivers/char/KeyMaps/config.in kconfig
	config/Configure config.in

menuconfig: elks/arch/i86/drivers/char/KeyMaps/config.in kconfig
	config/Menuconfig config.in
