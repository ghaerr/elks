#
#	Makefile for elksemu.
#

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

# Use BCC to make a tiny static a.out version.
ifeq ($(CC),bcc)
CFLAGS=-Ml -ansi -s $(DEFS)
endif
ifeq ($(CC),ncc)
CFLAGS=-Ml -ansi -s $(DEFS)
endif

# Default
ifeq ($(CFLAGS),)
CFLAGS=-O $(DEFS)
endif

# Turn on elkemu's strace like facility.
# DEFS=-DDEBUG

# For gcc making a.out with a basically ELF compiler
# CFLAGS=-O2 -fno-strength-reduce -b i486-linuxaout -N -s -static

OBJ=elks.o elks_sys.o elks_signal.o minix.o

elksemu:	$(OBJ)
		$(CC) $(CFLAGS) -o $@ $^

elks_sys.o:	call_tab.v defn_tab.v efile.h
$(OBJ):		elks.h

ifdef PREFIX
INSTALLED_CALL_TAB = $(PREFIX)/share/misc/elks/call_tab.v
INSTALLED_DEFN_TAB = $(PREFIX)/share/misc/elks/defn_tab.v
else
# Try to auto-detect the locations of the system call tables call_tab.v and
# defn_tab.v, if they are not already given by means of $(PREFIX).
#
# ia16-elf-gcc's target libraries are expected to go in PREFIX/ia16-elf/
# lib/, while the syscall tables should be in PREFIX/share/misc/elks/
# (PREFIX = ia16-elf-gcc's installation prefix).  So looking for "libraries"
# in ../../share/misc/elks/ relative to GCC's library directories should
# lead us on the right track.
INSTALLED_CALL_TAB:=$(shell \
		ia16-elf-gcc -print-file-name=../../share/misc/elks/call_tab.v)
INSTALLED_DEFN_TAB:=$(shell \
		ia16-elf-gcc -print-file-name=../../share/misc/elks/defn_tab.v)
ifeq "" "$(INSTALLED_CALL_TAB)"
$(error cannot find call_tab.v from elks-libc installation)
endif
ifeq "../../share/misc/elks/call_tab.v" "$(INSTALLED_CALL_TAB)"
$(error cannot find call_tab.v from elks-libc installation)
endif
ifeq "" "$(INSTALLED_DEFN_TAB)"
$(error cannot find defn_tab.v from elks-libc installation)
endif
ifeq "../../share/misc/elks/defn_tab.v" "$(INSTALLED_DEFN_TAB)"
$(error cannot find defn_tab.v from elks-libc installation)
endif
endif

call_tab.v:	$(INSTALLED_CALL_TAB)
	cp $< $@
defn_tab.v:	$(INSTALLED_DEFN_TAB)
	cp $< $@

efile.h: $(TOPDIR)/elkscmd/rootfs_template/lib/liberror.txt
	sh mkefile $<

dummy:

# The kernel patch or module _requires_ this location but binfmt-misc is easy
# to redirect.
install: elksemu
	install -d $(DIST)/lib
	install -s -o root -g root -m 4555 elksemu $(DIST)/lib/elksemu

clean realclean:
	rm -f $(OBJ) binfmt_elks.o elksemu call_tab.v defn_tab.v efile.h

module: binfmt_elks.o

# HOW to compile the module...
# BUT remember you don't need it for a recent 2.1.X; use binfmt_misc.

# This matches my compile (2.0.x); yours may be different.
MODCFLAGS=-D__KERNEL__ -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer \
          -fno-strength-reduce -pipe -m486 -DCPU=486 -DMODULE -DMODVERSIONS \
	  -include /usr/include/linux/modversions.h

binfmt_elks.o:	binfmt_elks.c
	gcc -c $(MODCFLAGS) binfmt_elks.c

# This is another option
#MODCFLAGS=-O -fomit-frame-pointer -DCPU=386 -D__KERNEL__ -DMODULE
# Or this ...
#MODCFLAGS=-O -fomit-frame-pointer -D__KERNEL__ -DMODULE -DCONFIG_MODVERSIONS

