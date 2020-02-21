# Makefile of /libc/stdio module

include $(TOPDIR)/libc/Makefile.inc

SRCS = \
	stdio.c \
	__fopen.c \
	__io_init.c \
	__io_list.c \
	fclose.c \
	fdopen.c \
	fflush.c \
	fgetc.c \
	fgets.c \
	fopen.c \
	fputc.c \
	fputs.c \
	fread.c \
	freopen.c \
	fscanf.c \
	fseek.c \
	ftell.c \
	fwrite.c \
	puts.c \
	gets.c \
	printf.c \
	putchar.c \
	rewind.c \
	scanf.c \
	setbuffer.c \
	setvbuf.c \
	sscanf.c \
	stderr.c \
	stdin.c \
	stdout.c \
	printf.c \
	putchar.c \
	ungetc.c \
	vfscanf.c \
	vscanf.c \

OBJS = $(SRCS:.c=.o)

$(OBJS): $(SRCS)

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
