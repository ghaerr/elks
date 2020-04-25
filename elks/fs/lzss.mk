LZSS	= ../lib/lzss
CFLAGS	+= -I$(LZSS)
OBJS	+= $(LZSS)/decode/get.o $(LZSS)/decode/init.o $(LZSS)/getbit.o
