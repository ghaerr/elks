BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

OBJS = err.o daemon.o

all: libbsd2.a

libbsd2.a: $(OBJS)
	ar -rc $(TARGET) $(OBJS)

clean:
	rm -rf core libbsd2.a $(OBJS)
