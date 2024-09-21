BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

###############################################################################

OBJS = \
	tiny_vfprintf.o \
	# END

CFLAGS += -fno-instrument-functions -fno-instrument-functions-simple

all: $(OBJS)

clean:
	$(RM) *.o
