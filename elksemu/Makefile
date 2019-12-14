#
#	Makefile for elksemu.
#

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

ifneq "" "$(PREFIX)"
call_tab.v:	$(PREFIX)/share/misc/elks/call_tab.v
	cp $< $@
defn_tab.v:	$(PREFIX)/share/misc/elks/defn_tab.v
	cp $< $@
endif

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

