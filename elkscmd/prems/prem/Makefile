BASEDIR = ../..
include $(BASEDIR)/Make.defs
include $(BASEDIR)/Make.rules

LOCALFLAGS=-I.

DEST          = ..

EXTHDRS       =

HDRS          = arbo.h bit.h

ELHDRS        = $(HDRS) elarbo.h elbit.h

ENHDRS        = $(HDRS) enarbo.h enbit.h

LIBRARY       = prem.a

MAKEFILE      = makefile

OBJS          = ela_komun.o \
		elbit.o \
		eli_komun.o \
		elig.o \
		ellit.o \
		ena_komun.o \
		enbit.o \
		eni_komun.o \
		enig.o \
		enlit.o \
		kre_hfoli.o \
		kre_nod.o \
		mkre_foli.o \
		mkre_litf.o \
		mkre_nod.o \
		sekv_niv.o \
		serch.o \
		x_kod.o \
		x_malkod.o \
		b_prem.o \
		b_malpre.o

PRINT         = pr

SRCS          =

all:            $(LIBRARY)

$(LIBRARY):     $(OBJS)
		@echo -n "Loading $(LIBRARY) ... "
		@$(AR) crsu $(LIBRARY) $(OBJS)
		@echo "done"

ela_komun.o:    ela_komun.c $(ELHDRS)

kre_hfoli.o:    kre_hfoli.c $(ELHDRS)

kre_nod.o:      kre_nod.c $(ELHDRS)

sekv_niv.o:     sekv_niv.c $(ELHDRS)

serch.o:        serch.c $(ELHDRS)

elbit.o:        elbit.c $(ELHDRS)

eli_komun.o:    eli_komun.c $(ELHDRS)

elig.o:         elig.c $(ELHDRS)

ellit.o:        ellit.c $(ELHDRS)

x_kod.o:        x_kod.c $(ELHDRS)

ena_komun.o:    ena_komun.c $(ENHDRS)

mkre_foli.o:    mkre_foli.c $(ENHDRS)

mkre_litf.o:    mkre_litf.c $(ENHDRS)

mkre_nod.o:     mkre_nod.c $(ENHDRS)

enbit.o:        enbit.c $(ENHDRS)

eni_komun.o:    eni_komun.c $(ENHDRS)

enig.o:         enig.c $(ENHDRS)

enlit.o:        enlit.c $(ENHDRS)

x_malkod.o:     x_malkod.c $(ENHDRS)

b_prem.o:       b_prem.c $(ELHDRS)

b_malpre.o:     b_malpre.c $(ENHDRS)

clean:;         @rm -f $(OBJS) $(LIBRARY)

depend:;        @mkmf -f $(MAKEFILE) LIBRARY=$(LIBRARY) DEST=$(DEST)

extract:;       @$(AR) xo $(DEST)/$(LIBRARY)
		@rm -f __.SYMDEF

index:;         @ctags -wx $(HDRS) $(SRCS)

install:        $(LIBRARY)
		@echo Installing $(LIBRARY) in $(DEST)
		@mv $(LIBRARY) $(DEST)
		@ranlib $(DEST)/$(LIBRARY)

library:        $(LIBRARY)

print:;         @$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:         $(DEST)/$(LIBRARY)

$(DEST)/$(LIBRARY): $(SRCS) $(HDRS) $(EXTHDRS)
		@-$(AR) xo $(DEST)/$(LIBRARY)
		@make -f $(MAKEFILE) DEST=$(DEST) install clean
###
