# PAINT Master Makefile

ALL = gcc

ifdef WATCOM
ALL += owc
endif

ifdef C86
#ALL += c86
endif

all: $(ALL)

gcc:
	make -f Makefile.gcc all

owc:
	make -f Makefile.owc all

c86:
	make -f Makefile.c86 all

clean:
	make -f Makefile.gcc clean
ifdef WATCOM
	make -f Makefile.owc clean
endif
ifdef C86
	make -f Makefile.c86 clean
endif
