# Makefile of /libc/stdio module

include $(TOPDIR)/libc/Makefile.inc

OBJS = \
	stdio.o \
	__fopen.o \
	__fp_print.o \
	__fp_print_func.o \
	__io_init.o \
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
	gets.o \
	printf.o \
	putchar.o \
	rewind.o \
	scanf.o \
	setbuffer.o \
	setvbuf.o \
	sprintf.o \
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

all: out.a

out.a: $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $^

clean:
	rm -f *.[ao]
