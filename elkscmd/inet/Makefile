BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

# All subdirectories to build & clean

SUBDIRS = \
    httpd \
    nettools \
    telnet \
    telnetd \
    tinyirc \
    urlget \
    # EOL

# Directories to install

INSTDIRS =

ifdef CONFIG_APP_HTTPD
INSTDIRS += httpd
endif

ifdef CONFIG_APP_NET_TOOLS
INSTDIRS += nettools
endif

ifdef CONFIG_APP_TELNET
INSTDIRS += telnet
endif

ifdef CONFIG_APP_TELNETD
INSTDIRS += telnetd
endif

ifdef CONFIG_APP_TINYIRC
INSTDIRS += tinyirc
endif

ifdef CONFIG_APP_URLGET
INSTDIRS += urlget
endif

all:
	for i in $(SUBDIRS); do $(MAKE) -C $$i all || exit 1 ; done

install:
	for i in $(INSTDIRS); do $(MAKE) -C $$i install || exit 1; done

clean:
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean || exit 1; done
