ROOT	= ..
ELKS	= $(ROOT)/elks
LIBC	= $(ROOT)/libc

NAME	= busyelks

OUT	= $(NAME)

CC	= ia16-elf-gcc

FLAGS	= \
	-mcmodel=small \
	-mno-segment-relocation-stuff \
	-mtune=i8086

CFLAGS	+= \
	$(FLAGS) \
	-fno-inline \
	-D_POSIX_SOURCE \
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

all: $(OUT)

$(OUT):	$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(OBJS)

doc:	README.html

README.html:	README
	asciidoc README
