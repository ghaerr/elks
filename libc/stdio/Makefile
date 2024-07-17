# Makefile of /libc/stdio module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

CFLAGS	+= -DL_ftell

OBJS = \
	init.o \
	__fopen.o \
	fclose.o \
	fdopen.o \
	fflush.o \
	fgetc.o \
	fgets.o \
	fopen.o \
	fprintf.o \
	fputc.o \
	fputs.o \
	fread.o \
	freopen.o \
	fscanf.o \
	fseek.o \
	ftell.o \
	fwrite.o \
	puts.o \
	getdelim.o \
	getline.o \
	gets.o \
	printf.o \
	putchar.o \
	rewind.o \
	scanf.o \
	setbuffer.o \
	setvbuf.o \
	sprintf.o \
	snprintf.o \
	sscanf.o \
	stderr.o \
	stdin.o \
	stdout.o \
	ungetc.o \
	vfprintf.o \
	vfscanf.o \
	vprintf.o \
	vscanf.o \
	vprintf.o \
	vsprintf.o \
	vsnprintf.o \
	# end of list

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
