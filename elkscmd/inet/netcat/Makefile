# Makefile for netcat

BASEDIR=../..

include $(BASEDIR)/Makefile-rules

###############################################################################

LDFLAGS += -maout-heap=128 -maout-stack=1024

###############################################################################

TARGET = netcat
SRC    = netcat.c
OBJ    = netcat.o

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)/bin
	install -m 755 $(TARGET) $(DESTDIR)/bin/$(TARGET)
	ln -sf $(TARGET) $(DESTDIR)/bin/nc

clean:
	$(RM) $(TARGET) $(OBJ)
