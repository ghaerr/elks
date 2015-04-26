CFLAGS        = -I../H

DEST          = ..

EXTHDRS       = ../H/arbo.h \
		../H/bit.h \
		../H/enarbo.h \
		../H/enbit.h \
		/usr/include/stdio.h

HDRS          =

LIBRARY       = enig.a

MAKEFILE      = makefile

OBJS          = enbit.o

PRINT         = pr

SRCS          = enbit.c

all:            $(LIBRARY)

$(LIBRARY):     $(OBJS)
		@echo -n "Loading $(LIBRARY) ... "
		@ar cru $(LIBRARY) $(OBJS)
		@echo "done"

clean:;         @rm -f $(OBJS)

depend:;        @mkmf -f $(MAKEFILE) LIBRARY=$(LIBRARY) DEST=$(DEST)

extract:;       @ar xo $(DEST)/$(LIBRARY)
		@rm -f __.SYMDEF

index:;         @ctags -wx $(HDRS) $(SRCS)

install:        $(LIBRARY)
		@echo Installing $(LIBRARY) in $(DEST)
		@mv $(LIBRARY) $(DEST)
		@ranlib $(DEST)/$(LIBRARY)
		@echo "done"

library:        $(LIBRARY)

print:;         @$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:         $(DEST)/$(LIBRARY)

$(DEST)/$(LIBRARY): $(SRCS) $(HDRS) $(EXTHDRS)
		@-ar xo $(DEST)/$(LIBRARY)
		@make -f $(MAKEFILE) DEST=$(DEST) install clean
###
enbit.o: ../H/enbit.h ../H/bit.h ../H/enarbo.h ../H/arbo.h \
	/usr/include/stdio.h
