# Makefile of ELKS C Library

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

include Makefile.inc

# Defines

SUBDIRS = \
	asm \
	error \
	gcc \
	getent \
	malloc \
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
#   * gcc has separate -melks-libc multilibs (and supports -melks-libc).
#   * In a slightly older (pre-April 2019) arrangement, gcc supports -melks,
#     and possibly -melks-libc, but does not have separate -melks-libc
#     multilibs.
#
# For the first case, arrange to build elks-libc for the available
# -melks-libc multilib settings.  Furthermore, if $(DESTDIR) is defined, add
# an `install' makefile rule which will install the elks-libc files under
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
ELKSLIBCMULTIS:=$(strip $(foreach ml,$(ALLMULTIS), \
    $(if $(findstring @melks-libc@,$(lastword $(subst ;, ,$(ml)))@),$(ml))))

ifneq "" "$(ELKSLIBCMULTIS)"
BUILDMULTIS:=$(ELKSLIBCMULTIS)
MAINMULTI:=$(filter %;@melks-libc,$(ELKSLIBCMULTIS))
else
MAINMULTI:=$(filter .;,$(ALLMULTIS))
BUILDMULTIS:=$(MAINMULTI)
override DESTDIR=
endif

MAINMULTISUBDIR=$(firstword $(subst ;, ,$(MAINMULTI)))

# For now do not build multilibs for the regparmcall calling convention, as
# the assembly code in elks-libc does not yet support it.  TODO.  -- tkchia
BUILDMULTIS:=$(strip \
    $(foreach ml,$(BUILDMULTIS), \
	$(if $(findstring @mregparmcall@,$(ml)@),,$(ml))))
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

$(LIBC): $(SUBDIRSINMULTILIB)
	mkdir -p $(@D)
	set -e; \
	( \
		echo CREATE $(LIBC).tmp; \
		for s in $^; \
			do echo ADDLIB "$$s"/out.a; done; \
		echo SAVE \
	) | $(AR) -M
	mv $(LIBC).tmp $(LIBC)

$(CRT0): crt0.S

$(SUBDIRSINMULTILIB) : $(TOPDIR)/libc/build-ml/$(MULTISUBDIR)/%: %
	mkdir -p $@
	$(MAKE) -C $@ VPATH=$(abspath $<) -f $(abspath $<)/Makefile all
# Use .PHONY to force recursive `make' in all subdirectories for each multilib
.PHONY: $(SUBDIRSINMULTILIB)

$(SUBDIRS):

# Install one particular multilib variant of libc.a, crt0.o, and include files.
# TODO: tidy up the include file installation?
ifneq "" "$(DESTDIR)"
INSTALL_DATA = install -c -m 644
MULTIINCDIR = $(DESTDIR)/ia16-elf/lib/$(MULTISUBDIR)/include
.PHONY: install
install: $(LIBC)
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
	      $(patsubst include/%,$(MULTIINCDIR)/%, \
		  $(wildcard include/*.h include/asm/*.h include/sys/*.h \
			     include/arpa/*.h include/netinet/*.h)) \
	      $(patsubst $(TOPDIR)/elks/include/%,$(MULTIINCDIR)/%, \
		  $(wildcard $(TOPDIR)/elks/include/arch/*.h \
			     $(TOPDIR)/elks/include/linuxmt/*.h))
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
	dest=$(DESTDIR)/share/misc/elks; \
	case $@ in \
	    install) \
		mkdir -p "$$dest"; \
		cp build-ml/$(MAINMULTISUBDIR)/system/call_tab.v \
		   build-ml/$(MAINMULTISUBDIR)/system/defn_tab.v "$$dest";; \
	    *) \
		$(RM) "$$dest"/call_tab.v "$$dest"/defn_tab.v; \
		rmdir -p "$$dest" || true;; \
	esac
endif

endif

.PHONY: clean

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done
	rm -rf build-ml *.o libc.a
