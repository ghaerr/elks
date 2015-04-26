BASEDIR = ../..
include $(BASEDIR)/Make.defs
include $(BASEDIR)/Make.rules

CFLAGS        = -I. -I../prem -O $(CFLBASE)

DEST          = .

HDRS          = ../prem/prem.h \
		pres.h

LDFLAGS       = -s # -n

LIBS          = ../prem/prem.a

XLIBS         = ../prem/xprem.a

LINKER        = $(CC)

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
		$(LINKER) $(LDFLAGS) $(POBJS) $(LIBS) -o pres
		@echo "done"

xpres:          $(POBJS) $(XLIBS)
		@echo -n "Loading xpres ... "
		@$(LINKER) $(LDFLAGS) $(POBJS) $(XLIBS) -o xpres
		@echo "done"

flparc:         $(FOBJS) $(LIBS)
		@echo -n "Loading flparc ... "
		@$(LINKER) $(LDFLAGS) $(FOBJS) $(LIBS) -o flparc
		@echo "done"

clean:;         @rm -f $(OBJS) pres flparc

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
		@make -f $(MAKEFILE) DEST=$(DEST) install
###
flparc.o: ../prem/prem.h
pres.o: ../prem/prem.h pres.h
