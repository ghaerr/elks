# Makefile of /libc/termcap module

include $(TOPDIR)/libc/Makefile.inc

LIB	= out.a

OBJS = \
	entry.o \
	find.o \
	memout.o \
	tgetent.o \
	tgetflag.o \
	tgetnum.o \
	tgetst1.o \
	tgetstr.o \
	tgoto.o \
	tparam.o \
	tparam1.o \
	tputs.o \
	xmalloc.o \
	xrealloc.o \

#CFLAGS	+= -DDEBUG
#CFLAGS	+= -DTIOCGWINSZ
#CFLAGS	+= -DNO_ARG_ARRAY
#CFLAGS	+= -DHAVE_CONFIG_H

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

TC_OBJS = termcap.o

termcap: $(TC_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

TP_OBJS = tp-main.o memout.o tparam1.o xmalloc.o xrealloc.o

tparam: $(TP_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

distclean: clean
	$(RM) $(LIB) termcap tparam

clean::
	$(RM) *.[aod]
