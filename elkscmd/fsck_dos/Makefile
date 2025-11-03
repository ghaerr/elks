BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS = -Dlint -DELKS=1 -D__huge=
# remove next line when FAT32 supported
LOCALFLAGS += -Wno-shift-count-overflow

###############################################################################

PRGS = fsck-dos
OBJS = main.o boot.o check.o dir.o fat.o

all: $(PRGS)

$(PRGS): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) -maout-heap=0xffff $^ $(LDLIBS)

install: all
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) *.o $(PRGS) *.obj *.os2
