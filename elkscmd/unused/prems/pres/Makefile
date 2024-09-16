BASEDIR = ../..
include $(BASEDIR)/Make.defs
include $(BASEDIR)/Make.rules

LOCALFLAGS= -I. -I../prem

DEST          = .

HDRS          = ../prem/prem.h \
		pres.h

LIBS          = ../prem/*.o

XLIBS         = ../prem/xprem.a

MAKEFILE      = makefile

FOBJS         = flparc.o

POBJS         = pres.o

OBJS          = $(POBJS) $(FOBJS)

PRINT         = pr

SRCS          = flparc.c \
		pres.c

all:            pres flparc

pres:           $(POBJS) $(LIBS)
		@echo -n "Loading pres ... "
		$(LD) $(LDFLAGS) $(POBJS) $(LIBS) -o pres $(LDLIBS)
		@echo "done"

xpres:          $(POBJS) $(XLIBS)
		@echo -n "Loading xpres ... "
		@$(CC) $(LDFLAGS) $(POBJS) $(XLIBS) -o xpres
		@echo "done"

flparc:         $(FOBJS) $(LIBS)
		@echo -n "Loading flparc ... "
		@$(LD) $(LDFLAGS) $(FOBJS) $(LIBS) -o flparc $(LDLIBS)
		@echo "done"

clean:;         @$(RM) $(OBJS) pres flparc

depend:;        @mkmf -f $(MAKEFILE) PROGRAM=$(PROGRAM) DEST=$(DEST)

index:;         @ctags -wx $(HDRS) $(SRCS)

install:        $(PROGRAM)
		@echo Installing $(PROGRAM) in $(DEST)
		@install -s $(PROGRAM) $(DEST)

print:;         @$(PRINT) $(HDRS) $(SRCS)

program:        $(PROGRAM)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:         $(DEST)/$(PROGRAM)

$(DEST)/$(PROGRAM): $(SRCS) $(LIBS) $(HDRS)
		@$(MAKE) -f $(MAKEFILE) DEST=$(DEST) install
###
flparc.o: ../prem/prem.h
pres.o: ../prem/prem.h pres.h
