ROOT	= ..
ELKS	= $(ROOT)/elks
LIBC	= $(ROOT)/libc

NAME	= busyelks

OUT	= $(NAME)
OUTINST	= $(ROOT)/elkscmd/rootfs_template/bin/$(OUT)

CC	= ia16-elf-gcc

FLAGS	= \
	-mcmodel=small \
	-mno-segment-relocation-stuff \
	-mtune=i8086

CFLAGS	+= \
	$(FLAGS) \
	-fno-inline \
	-D_POSIX_SOURCE \
	-I. \
	-I$(ROOT)/include \
	-I$(ELKS)/include \
	-I$(LIBC)/include \
	-D__ELKS__ \
	-DELKS_VERSION=\"0.3.0-prei86\"
CFLAGS	+= -Os
CFLAGS	+= -Wall

LD	= $(CC)
LDFLAGS	+= \
	$(FLAGS) \
	-melks-libc \

OBJS	+= main.o

include config.mk

ifdef CMD_basename
	CFLAGS	+= -DCMD_basename
	OBJS	+= cmd/basename.o
	NEED_stripslash	= yes
endif

ifdef CMD_dirname
	CFLAGS	+= -DCMD_dirname
	OBJS	+= cmd/dirname.o
	NEED_stripslash	= yes
endif

ifdef CMD_false
	CFLAGS	+= -DCMD_false
endif

ifdef CMD_true
	CFLAGS	+= -DCMD_true
endif

ifdef NEED_stripslash
	OBJS	+= lib/stripslash.o
endif

all: $(OUT)

$(OUT):	$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(OBJS): config.mk

clean:
	$(RM) $(OBJS)

doc:	README.html

README.html:	README
	asciidoc README

install:	$(OUTINST)

$(OUTINST):	$(OUT)
	cp $< $@
