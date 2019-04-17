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

ifdef MULTISUBDIR

# Build one particular multilib variant of libc.a.  $(MULTISUBDIR) gives the
# multilib variant to build for, and $(MULTILIB) gives the C compiler flags
# to pass to GCC.
all: $(LIBC) build-ml/$(MULTISUBDIR)/crt0.o

SUBDIRS_IN_MULTILIB = $(SUBDIRS:%=build-ml/$(MULTISUBDIR)/%)

$(LIBC): $(SUBDIRS_IN_MULTILIB)
	mkdir -p $(@D)
	set -e; \
	( \
		echo CREATE $(LIBC).tmp; \
		for s in $^; \
			do echo ADDLIB "$$s"/out.a; done; \
		echo SAVE \
	) | $(AR) -M
	mv $(LIBC).tmp $(LIBC)

build-ml/$(MULTISUBDIR)/crt0.o: crt0.S

build-ml/$(MULTISUBDIR)/%: %
	mkdir -p $@
	$(MAKE) -C $@ VPATH=$(abspath $<) -f $(abspath $<)/Makefile all

$(SUBDIRS):

else

# This currently just builds the default multilib.
# TODO: if $(MULTISUBDIR) is undefined, then build all libc.a multilibs.

.PHONY: $(LIBC)

all: $(LIBC)

$(LIBC):
	set -e; \
	save_ifs="$$IFS"; \
	$(CC) -print-multi-lib | fgrep -x '.;' | \
	while read -r line; do \
		IFS=';'; \
		set -- $$line; \
		dir="$$1"; \
		flags="$$2"; \
		IFS="$$save_ifs"; \
		flags="`echo "$$flags" | sed 's,@, -,g'`"; \
		$(MAKE) MULTISUBDIR="$$dir" MULTILIB="$$flags" all; \
	done
	cp -u build-ml/libc_elks.a $(LIBC)

endif

.PHONY: clean

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done
	rm -rf build-ml *.o libc.a
