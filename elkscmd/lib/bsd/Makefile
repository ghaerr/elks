include ../../Make.defs

OBJS = err.o daemon.o
TARGET = libbsd2.a

$(TARGET): $(OBJS)
	ar -rc $(TARGET) $(OBJS)

clean:
	rm -rf *.o $(TARGET)
