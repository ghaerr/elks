BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

DIRS =

ifdef CONFIG_APP_HTTPD
DIRS += httpd
endif

ifdef CONFIG_APP_NET_TOOLS
DIRS += nettools
endif

ifdef CONFIG_APP_TELNET
DIRS += telnet
endif

ifdef CONFIG_APP_TELNETD
DIRS += telnetd
endif

ifdef CONFIG_APP_TINYIRC
DIRS += tinyirc
endif

ifdef CONFIG_APP_URLGET
DIRS += urlget
endif

all:
	for i in $(DIRS); do make -C $$i all || exit 1 ; done

install:
	for i in $(DIRS); do make -C $$i install || exit 1; done

clean:
	for i in $(DIRS); do make -C $$i clean || exit 1; done
