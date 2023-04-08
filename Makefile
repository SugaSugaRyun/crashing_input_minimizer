CC=gcc
CFLAGS=-g -Wall
OBJS=cimin.o
TARGET=cimin

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS)

cimin.o: cimin.c

clean:
	rm -f *.o
	rm -f $(TARGET)
