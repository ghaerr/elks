BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS = -DNO_LP -DELKS
#LOCALFLAGS = -DNO_LP -DELKS -DNODEBUG

###############################################################################

PRGS = ekermit
OBJS = main.o kermit.o elksio.o

all: $(PRGS)

$(PRGS): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^ $(LDLIBS)

install: all
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) *.o $(PRGS)
