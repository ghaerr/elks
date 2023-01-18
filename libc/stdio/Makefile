# Makefile of /libc/stdio module

include $(TOPDIR)/libc/Makefile.inc

CFLAGS	+= -DL_ftell

OBJS = \
	init.o \
	__fopen.o \
	__io_list.o \
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
	putchar.o \
	ungetc.o \
	vfprintf.o \
	vfscanf.o \
	vprintf.o \
	vscanf.o \
	vprintf.o \
	vsprintf.o \
	vsnprintf.o \
	# end of list

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	$(RM) *.[aod]
