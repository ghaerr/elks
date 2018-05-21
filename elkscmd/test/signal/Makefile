# Makefile for signal test

BASEDIR=../..

include $(BASEDIR)/Make.defs

PGM = test_signal

SRCS = $(PGM).c

OBJS = $(SRCS:.c=.o)


include $(BASEDIR)/Make.rules

all: $(PGM)

$(PGM): $(OBJS)

install: $(PGM)
	sudo install $(PGM) $(TARGET_MNT)/bin

clean: 
	rm -f $(OBJS) $(PGM)
