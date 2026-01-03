BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS = -DNO_LP -DELKS
LOCALFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -Wno-pointer-sign
#LOCALFLAGS += -DNODEBUG

###############################################################################

PRGS = ekermit
OBJS = main.o kermit.o elksio.o

all: $(PRGS)

$(PRGS): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^ $(LDLIBS)

HOSTPRGS = hostek
.PHONY: host
host: $(HOSTPRGS)

HOSTSRC = main.c kermit.c elksio.c
HOSTCFLAGS += -DNO_LP -DELKS
HOSTCFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -Wno-pointer-sign

hostek: $(HOSTSRC)
	$(HOSTCC) $(HOSTCFLAGS) $(HOSTSRC) -o $@

install: all
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) *.o $(PRGS) $(HOSTPRGS)
