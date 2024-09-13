# Object file manipulation, conversion and display utilities
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	=

CLEANME 	= ../bin/objdump86 ../bin/omfdump ../bin/omf2elf \
	../bin/os2toelks ../bin/os2dump \
	../bin/ewcc ../bin/ewlink

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

#########################################################################
# Commands.

CFLAGS += -Wall -Wextra -O3

all: ../bin/objdump86 ../bin/omf2elf ../bin/omfdump ../bin/os2toelks ../bin/os2dump ../bin/ewcc ../bin/ewlink

../bin/objdump86: objdump86.o
	$(CC) -o ../bin/objdump86 $(CFLAGS) $^

../bin/omfdump: omfdump.o
	$(CC) -o ../bin/omfdump $(CFLAGS) $^

../bin/omf2elf: omf2elf.o
	$(CC) -o ../bin/omf2elf $(CFLAGS) $^

../bin/os2toelks: os2toelks.o
	$(CC) -o ../bin/os2toelks $(CFLAGS) $^

../bin/os2dump: os2dump.o
	$(CC) -o ../bin/os2dump $(CFLAGS) $^

../bin/ewcc: ewcc
	cp -p ewcc ../bin

../bin/ewlink: ewlink
	cp -p ewlink ../bin

#########################################################################
### Dependencies:
