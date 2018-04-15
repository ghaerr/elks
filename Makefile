#########################################################################
# Variables

# Specify the configuration shell to use.

CFG_SHELL	:= $(shell \
	     if [ -x "$$bash" ]; \
		then echo $$bash ; \
		else if [ -x /bin/bash ]; \
			then echo /bin/bash ; \
			else if [ -x /usr/bin/bash ]; \
				then echo /usr/bin/bash ; \
				else if [ -x /usr/local/bin/bash ]; \
					then echo /usr/local/bin/bash ; \
					else echo sh ; \
				     fi ; \
			     fi ; \
		     fi ; \
	     fi)

#########################################################################
# Rules

.PHONY: all clean kconfig defconfig config menuconfig

all: .config
	$(MAKE) -C elks all
	$(MAKE) -C elkscmd all

clean:
	$(MAKE) -C elks clean
	$(MAKE) -C elkscmd clean
	@echo
	@if [ ! -f .config ]; then \
	    echo ' * This system is not configured. You need to run' ;\
	    echo ' * `make config` or `make menuconfig` to configure it.' ;\
	    echo ;\
	fi
	
elks/arch/i86/drivers/char/KeyMaps/config.in:
	$(MAKE) -C elks/arch/i86/drivers/char/KeyMaps config.in

kconfig:
	$(MAKE) -C config all

defconfig:
	@yes '' | ${MAKE} config

config:	elks/arch/i86/drivers/char/KeyMaps/config.in kconfig
	$(CFG_SHELL) config/Configure config.in

menuconfig:	elks/arch/i86/drivers/char/KeyMaps/config.in kconfig
	$(CFG_SHELL) config/Menuconfig config.in

#########################################################################
