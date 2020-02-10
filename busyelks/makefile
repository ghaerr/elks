ROOT	= ..
ELKS	= $(ROOT)/elks
LIBC	= $(ROOT)/libc

NAME	= busyelks

OUT	= $(NAME)
INSTDIR	= $(ROOT)/elkscmd/rootfs_template

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

OBJS	+= main.o lib/bsearch.o

include config.mk

ifdef CMD_basename
	CFLAGS	+= -DCMD_basename
	OBJS	+= cmd/basename.o
	NEED_stripslash	= yes
endif

ifdef CMD_cal
	CFLAGS	+= -DCMD_cal
	OBJS	+= cmd/cal.o
endif

ifdef CMD_cat
	CFLAGS	+= -DCMD_cat
	OBJS	+= cmd/cat.o
endif

ifdef CMD_chgrp
	CFLAGS	+= -DCMD_chgrp
	OBJS	+= cmd/chgrp.o
endif

ifdef CMD_chmod
	CFLAGS	+= -DCMD_chmod
	OBJS	+= cmd/chmod.o
endif

ifdef CMD_chown
	CFLAGS	+= -DCMD_chown
	OBJS	+= cmd/chown.o
endif

ifdef CMD_cksum
	CFLAGS	+= -DCMD_cksum
	OBJS	+= cmd/cksum.o
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

install: busyelks.tar.xz
	tar Jxf $< -C $(INSTDIR)

busyelks.tar.xz: busyelks.fs $(OUT)
	b=`grep '/busyelks' < $<` \
	&& $(RM) -r _install \
	&& mkdir -p "`dirname "_install/$$b"`" \
	&& cp $(OUT) "_install/$$b" \
	&& grep '^[ 	]*CMD(' < main.c \
		| sed 's|^.*CMD([0 \t]*"||' \
		| sed 's|".*$$||' \
		| while read f; \
			do \
				if p=`grep "/$$f"` < $<; \
				then \
					mkdir -p "`dirname "_install/$$p"`"; \
					ln -sr "_install/$$b" "_install/$$p"; \
				fi; \
			done
	tar Jcf $@ -C _install .
	$(RM) -r _install
