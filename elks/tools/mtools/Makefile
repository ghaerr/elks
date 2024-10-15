# Build mtools-4.0.23
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR     = ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC      = N

CLEANDEP    =

CLEANME     = ../bin/mattrib ../bin/mcopy ../bin/mdir ../bin/mformat \
    ../bin/minfo ../bin/mmd ../bin/mshowfat

DEPEND      =

DISTFILES   =

NOINDENT    =

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

#########################################################################
# Commands.

all: mtools-4.0.23/Makefile
	make -C mtools-4.0.23
	cp mtools-4.0.23/mattrib ../bin
	cp mtools-4.0.23/mcopy ../bin
	cp mtools-4.0.23/mdir ../bin
	cp mtools-4.0.23/mformat ../bin
	cp mtools-4.0.23/minfo ../bin
	cp mtools-4.0.23/mmd ../bin
	cp mtools-4.0.23/mshowfat ../bin

mtools-4.0.23/Makefile:
	cd mtools-4.0.23; ./configure --disable-floppyd; cd ..

dokclean: doclean

doclean:
	make -C mtools-4.0.23 clean
