# Makefile for m4

BASEDIR = ..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALFLAGS =

###############################################################################

COMPRESS=../misc_utils/compress.host

OBJS=	eval.o expr.o look.o main.o misc.o serv.o

CLEANFILES= $(OBJS)

all:	m4

m4:	$(OBJS)
	$(CC) $(CFLAGS) -o m4 $(OBJS) $(LIBS)

install: m4
	$(INSTALL) m4 $(DESTDIR)/usr/bin

test: m4
	m4 < test.m4 > test_host_m4.txt
	elksemu ./m4 < test.m4 > test_elks_m4.txt
	diff -u test_host_m4.txt test_elks_m4.txt

clean:
	$(RM) m4 $(CLEANFILES)

