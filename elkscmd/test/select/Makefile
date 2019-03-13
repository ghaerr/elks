# Makefile for ethernet driver test

BASEDIR=../..

include $(BASEDIR)/Make.defs

PGM = test_select

SRCS = $(PGM).c

OBJS = $(SRCS:.c=.o)


include $(BASEDIR)/Make.rules

all: $(PGM)

$(PGM): $(OBJS)
	$(LD) $(LDFLAGS) -o $(PGM) $(OBJS) $(LDLIBS)

install: $(PGM)
	sudo install $(PGM) $(TARGET_MNT)/bin

clean: 
	rm -f $(OBJS) $(PGM)
