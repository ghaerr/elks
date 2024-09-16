BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

OBJS = \
	tiny_vfprintf.o \
	# END

CFLAGS += -fno-instrument-functions -fno-instrument-functions-simple

all: $(OBJS)

clean:
	$(RM) *.o
