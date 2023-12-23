# Makefile of ELKS C Library

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

include Makefile.inc

# Defines

SUBDIRS = \
	asm \
	ctype \
	error \
	gcc \
	getent \
	malloc \
	math \
	misc \
	net \
	regex \
	stdio \
	string \
	system \
	termcap \
	termios \
	time \
	# end of list

#SUBDIRS=list

# Rules

.PHONY: all $(SUBDIRS)

# Stuff for handling multilibs (https://wiki.gentoo.org/wiki/Multilib),
# which ia16-elf-gcc uses to support separate library files for multiple ABIs
# for the same architecture and OS.
#
# Much of the complexity in the code below is for processing the output of
# GCC's -print-multi-lib option.  `ia16-elf-gcc -print-multi-lib' prints
# something like this:
#
#	.;
#	...
#	rtd;@mrtd
#	...
#	regparmcall/any_186/size;@mregparmcall@march=any_186@Os
#	...
#
# The last line above means that the options `-mregparmcall -march=any_186
# -Os' (listed after the `;') will make GCC look in the subdirectories
# regparmcall/any_186/size/ within the library search path's directories,
# when looking for library files.  `.' in the first line is the subdirectory
# for default compiler options.
#
# Depending on the version of ia16-elf-gcc, there are two possibilities:
#   * gcc has separate elks-libc multilibs, and supports the option
#     -melks-libc and possibly also (post-May 2022) -mr=elks.
#   * In a slightly older (pre-April 2019) arrangement, gcc supports -melks,
#     and possibly -melks-libc, but does not have separate elks-libc
#     multilibs.
#
# For the first case, arrange to build elks-libc for the available elks-libc
# multilib settings.  Furthermore, if $(DESTDIR) is defined, add an
# `install' makefile rule which will install the elks-libc files under
# $(DESTDIR), such that they can be used from outside the ELKS source tree;
# and also add an `uninstall' rule to allow elks-libc to be cleaned away.
#
# For the second case, arrange to build elks-libc only for default compiler
# settings.  Installing the libraries outside the ELKS tree is not supported
# in this case.
#
# $(MAINMULTISUBDIR) gives the multilib subdirectory containing the files we
# want to get the generated syscall tables from --- which should be the same
# anyway across all multilibs.  $(MAINMULTISUBDIR) is expected to be either
# `elkslibc' (first case) or `.' (second case).
#	-- tkchia

ALLMULTIS:=$(strip $(shell $(CC) -print-multi-lib))
ELKSLIBCMULTIS:=$(sort $(foreach ml,$(ALLMULTIS), \
    $(if $(findstring elks-libc,$(ml))$(findstring elkslibc,$(ml)),$(ml))))

ifneq "" "$(ELKSLIBCMULTIS)"
BUILDMULTIS:=$(ELKSLIBCMULTIS)
MAINMULTI:=$(firstword $(foreach ml,$(ELKSLIBCMULTIS), \
    $(if $(findstring /,$(ml)),,$(ml))))
else
MAINMULTI:=$(filter .;,$(ALLMULTIS))
BUILDMULTIS:=$(MAINMULTI)
override DESTDIR=
endif

MAINMULTISUBDIR=$(firstword $(subst ;, ,$(MAINMULTI)))

ifeq "" "$(BUILDMULTIS)"
$(error no multilib variants to build for elks-libc)
endif
ifeq "" "$(MAINMULTISUBDIR)"
$(error no main multilib variant to use)
endif

ifneq "" "$(MULTISUBDIR)"

# Build one particular multilib variant of libc.a.  $(MULTISUBDIR) gives the
# multilib variant to build for, and $(MULTILIB) gives the C compiler flags
# to pass to GCC.
all: $(LIBC) $(CRT0)

SUBDIRSINMULTILIB = $(SUBDIRS:%=$(TOPDIR)/libc/build-ml/$(MULTISUBDIR)/%)
SUBLIBSINMULTILIB = $(SUBDIRSINMULTILIB:%=%/out.a)

# `ia16-elf-ar -M' errors out if the path argument to `CREATE' contains any
# weird characters such as `~'.  This caused problems when building elks-libc
# pre-releases on launchpad.net.  To work around this, make all paths in the
# `ia16-elf-ar -M' MRI script relative to $(TOPDIR).
$(LIBC): $(SUBLIBSINMULTILIB)
	mkdir -p $(@D)
	$(RM) -r $@.tmp
	set -e; \
	cd $(TOPDIR); \
	( \
		echo CREATE $(patsubst $(TOPDIR)/%,%,$(abspath $@.tmp)); \
		for s in $(patsubst $(TOPDIR)/%,%,$(abspath $^)); \
			do echo ADDLIB "$$s"; done; \
		echo SAVE \
	) | $(AR) -M
	mv $@.tmp $@

$(CRT0): crt0.S

$(SUBLIBSINMULTILIB) : $(TOPDIR)/libc/build-ml/$(MULTISUBDIR)/%/out.a: %
	mkdir -p $(@D)
	$(MAKE) -C $(@D) VPATH=$(abspath $<) -f $(abspath $<)/Makefile all
# Use .PHONY to force recursive `make' in all subdirectories for each multilib
.PHONY: $(SUBLIBSINMULTILIB)

$(SUBDIRS):

# Install one particular multilib variant of libc.a, crt0.o, and include files.
# TODO: tidy up the include file installation?
ifneq "" "$(DESTDIR)"
INSTALL_DATA = install -c -m 644
MULTIINCDIR = $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/include
.PHONY: install
install: $(LIBC) uninstall
	mkdir -p $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR) \
		 $(MULTIINCDIR)/asm $(MULTIINCDIR)/sys \
		 $(MULTIINCDIR)/arch $(MULTIINCDIR)/arpa \
		 $(MULTIINCDIR)/netinet $(MULTIINCDIR)/linuxmt
ifeq "" "$(filter -mcmodel=medium,$(MULTILIB))"
	$(INSTALL_DATA) $(LIBC) $(CRT0) \
	    $(TOPDIR)/elks/elks-small.ld $(TOPDIR)/elks/elks-tiny.ld \
	    $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)
else
	$(INSTALL_DATA) $(LIBC) $(CRT0) $(TOPDIR)/elks/elks-medium.ld \
	    $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)
endif
	$(INSTALL_DATA) include/*.h $(TOPDIR)/include/autoconf.h $(MULTIINCDIR)
	$(INSTALL_DATA) include/asm/*.h $(MULTIINCDIR)/asm
	$(INSTALL_DATA) include/sys/*.h $(MULTIINCDIR)/sys
	$(INSTALL_DATA) include/arpa/*.h $(MULTIINCDIR)/arpa
	$(INSTALL_DATA) include/netinet/*.h $(MULTIINCDIR)/netinet
	$(INSTALL_DATA) $(TOPDIR)/elks/include/arch/*.h $(MULTIINCDIR)/arch
	$(INSTALL_DATA) $(TOPDIR)/elks/include/linuxmt/*.h \
	    $(MULTIINCDIR)/linuxmt
uninstall:
	$(RM) $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/$(notdir $(LIBC)) \
	      $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/$(notdir $(CRT0)) \
	      $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/elks-medium.ld \
	      $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/elks-small.ld \
	      $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/elks-tiny.ld \
	      $(MULTIINCDIR)/autoconf.h \
	      $(patsubst include/%,$(MULTIINCDIR)/%,$(wildcard include/*.h))
	# Assume that all <asm/*>, <sys/*>, <arpa/*>, <netinet/*>, <arch/*>,
	# & <linuxmt/*> header files were installed from here (elks-libc) &
	# not from elsewhere.
	#
	# This also allows us to clean away obsolete internal header files
	# such as <linuxmt/arpa/inet.h> --- see discussion at
	# https://github.com/jbruchon/elks/pull/713 .
	#
	# This code will need to be adjusted if the assumption is ever broken.
	#	-- tkchia 20200818
	$(RM) -r $(MULTIINCDIR)/asm $(MULTIINCDIR)/sys $(MULTIINCDIR)/arpa \
		 $(MULTIINCDIR)/netinet $(MULTIINCDIR)/arch \
		 $(MULTIINCDIR)/linuxmt
endif

else

# $(MULTISUBDIR) is undefined.  Build all multilibs that we can build; then
# copy the library files for "default" settings into this directory.
.PHONY: all

all:
	set -e \
	$(foreach ml,$(BUILDMULTIS), ; \
		export MULTISUBDIR='$(firstword $(subst ;, ,$(ml)))'; \
		export MULTILIB='$(strip $(subst @, -, \
		    $(lastword $(subst ;, ,$(ml)))))'; \
		$(MAKE) all)

ifneq "" "$(DESTDIR)"
.PHONY: install uninstall
# Besides installing library files and header files, also install the system
# call tables call_tab.v and defn_tab.v in a reasonable place.  The elksemu
# build process needs these.
install uninstall:
	set -e \
	$(foreach ml,$(BUILDMULTIS), ; \
		export MULTISUBDIR='$(firstword $(subst ;, ,$(ml)))'; \
		export MULTILIB='$(strip $(subst @, -, \
		    $(lastword $(subst ;, ,$(ml)))))'; \
		$(MAKE) $@)
	set -e; \
	dest1=$(DESTDIR)/share/misc/elks; \
	dest2=$(DESTDIR)/ia16-elf/lib/rt-specs; \
	case $@ in \
	    install) \
		mkdir -p "$$dest1" "$$dest2"; \
		cp build-ml/$(MAINMULTISUBDIR)/system/call_tab.v \
		   build-ml/$(MAINMULTISUBDIR)/system/defn_tab.v "$$dest1"; \
		cp $(TOPDIR)/elks/r-elks.spec "$$dest2";; \
	    *) \
		$(RM) "$$dest1"/call_tab.v "$$dest1"/defn_tab.v \
		      "$$dest2"/r-elks.spec; \
		rmdir -p "$$dest1" "$$dest2" || true;; \
	esac
endif

endif

.PHONY: clean

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done
	rm -rf build-ml *.o libc.a
