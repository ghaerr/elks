BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

CFLAGS += -DELKS=1 -DUNIX=1 -DNDEBUG=1 -I..
LDFLAGS += -L..
LDLIBS += -lnano-X
BIN = ../bin/

###############################################################################

PROGS   = \
    $(BIN)nxclock   \
    $(BIN)nxtetris  \
    $(BIN)nxterm    \
    $(BIN)nxworld   \
    $(BIN)nxmine\
    $(BIN)nxdemo    \

all: $(PROGS)

$(BIN)nxclock: nxclock.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN)nxtetris: nxtetris.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN)nxterm: nxterm.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN)nxworld: nxworld.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp -p nxworld.map $(BIN)

$(BIN)nxmine: nxmine.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN)nxdemo: nxdemo.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(PROGS) *.o $(BIN)nxworld.map
