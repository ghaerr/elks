ELKS	= ../..
BASEDIR	= ../
include $(BASEDIR)/Make.defs
include $(BASEDIR)/Make.rules

# Install destination
DESTDIR		= $(TOPDIR)/target
ELKS_LIB86	= $(ELKS)/elks/arch/i86/lib/

NAME	= port

OUT	= $(NAME)
CFLAGS	+= -I$(ELKS)/elks/include/arch
OBJS	+= main.o

all: $(OUT)

$(OUT):	$(OBJS) $(LIBS)
	echo '[LD]	$@'
	$(LD) $(LDFLAGS) -o $@ $^

distclean: clean
	$(RM) *~ $(OUT)

clean::
	$(RM) $(OBJS)

install: $(OUT)
	cp $(OUT) $(DESTDIR)/bin
