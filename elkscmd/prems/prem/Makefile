BASEDIR = ../..
include $(BASEDIR)/Make.defs
include $(BASEDIR)/Make.rules
CFLAGS	= $(CFLBASE)

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
		@$(AR) cru $(LIBRARY) $(OBJS)
		@echo "done"

ela_komun.o:    ela_komun.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) ela_komun.c

kre_hfoli.o:    kre_hfoli.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) kre_hfoli.c

kre_nod.o:      kre_nod.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) kre_nod.c

sekv_niv.o:     sekv_niv.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) sekv_niv.c

serch.o:        serch.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) serch.c

elbit.o:        elbit.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) elbit.c

eli_komun.o:    eli_komun.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) eli_komun.c

elig.o:         elig.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) elig.c

ellit.o:        ellit.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) ellit.c

x_kod.o:        x_kod.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) x_kod.c

ena_komun.o:    ena_komun.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) ena_komun.c

mkre_foli.o:    mkre_foli.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) mkre_foli.c

mkre_litf.o:    mkre_litf.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) mkre_litf.c

mkre_nod.o:     mkre_nod.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) mkre_nod.c

enbit.o:        enbit.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) enbit.c

eni_komun.o:    eni_komun.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) eni_komun.c

enig.o:         enig.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) enig.c

enlit.o:        enlit.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) enlit.c

x_malkod.o:     x_malkod.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) x_malkod.c

b_prem.o:       b_prem.c $(ELHDRS)
		$(CC) -O -c -I. $(CFLAGS) b_prem.c

b_malpre.o:     b_malpre.c $(ENHDRS)
		$(CC) -O -c -I. $(CFLAGS) b_malpre.c

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
