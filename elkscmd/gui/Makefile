# PAINT Makefile for ELKS using IA16

############# Standard Section for IA16 C ##############
ifndef TOPDIR
$(error ELKS TOPDIR is not defined)
endif

CC = ia16-elf-gcc
LD = ia16-elf-gcc
CLBASE = -melks-libc -mtune=i8086 -mcmodel=small -mno-segment-relocation-stuff
CLBASE += -fno-inline -fno-builtin-printf -fno-builtin-fprintf
OPTFLAGS = -O2
OPTFLAGS += -fno-align-jumps
OPTFLAGS += -fno-align-functions
OPTFLAGS += -fno-align-loops
OPTFLAGS += -fno-align-labels
OPTFLAGS += -fira-region=one
OPTFLAGS += -fira-hoist-pressure
OPTFLAGS += -freorder-blocks-algorithm=simple
OPTFLAGS += -fno-reorder-blocks
OPTFLAGS += -fno-prefetch-loop-arrays
OPTFLAGS += -fno-tree-ch
WARNINGS = -Wall -Wextra -Wtype-limits -Wno-unused-parameter -Wno-sign-compare
INCLUDES = -I$(TOPDIR)/include -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include
DEFINES = 
CFLAGS = $(OPTFLAGS) $(CLBASE) $(WARNINGS) $(INCLUDES) $(DEFINES) $(LOCALFLAGS)
LDFLAGS = $(CLBASE)
LDLIBS =

OBJS = $(SRCS:.c=.oaj)
%.oaj: %.c
	$(CC) $(CFLAGS) -c -o $*.oaj $<

%.oaj: %.S
	$(CC) $(CFLAGS) -c -o $*.oaj $<

############# End of Standard Section ##############

BINDIR = .
LOCALFLAGS =
PROG = $(BINDIR)/paint
SRCS = app.c gui.c input.c render.c event.c mouse.c graphics.c drawbmp.c cursor.c \
    drawscanline.c
OBJS += vga-ia16.oaj

all: $(PROG)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(PROG) *.oaj
