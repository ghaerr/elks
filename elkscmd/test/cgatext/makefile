ELKS	= ../..
BASEDIR	= ../
include $(BASEDIR)/Makefile-rules

# Install destination
DESTDIR		= $(TOPDIR)/target
ELKS_LIB86	= $(ELKS)/elks/arch/i86/lib/

NAME	= cgatext

OUT	= $(NAME)
CFLAGS	+= -I$(ELKS)/elks/include/arch
OBJS	+= main.o
LIBS	= $(ELKS_LIB86)/lib86.a

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
