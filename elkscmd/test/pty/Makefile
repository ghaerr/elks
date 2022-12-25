# Makefile for pseudo tty test

BASEDIR=../..

include $(BASEDIR)/Make.defs

PGM = test_pty

SRCS = $(PGM).c
OBJS = $(SRCS:.c=.o)

include $(BASEDIR)/Make.rules

all: $(PGM)

$(PGM):	$(OBJS)
	$(LD) $(LDFLAGS) -o $(PGM) $(OBJS) $(LDLIBS)

install: $(PGM)
	$(INSTALL) $(PGM) $(DESTDIR)/bin

clean: 
	rm -f $(OBJS) $(PGM)
