PROG = node
CC = gcc
FLAGS = -Wall -Wno-unused-result -Wno-unused-function -O3 -std=c99 -ggdb -c
OBJECTS = main.o windowbuffer.o circularbuffer.o datalink.o transport.o network.o
HEADERS = node.h windowbuffer.h circularbuffer.h datalink.h transport.h network.h

all: $(OBJECTS)
	$(CC) -o $(PROG) $(OBJECTS)

%.o: %.c $(HEADERS)
	$(CC) $(FLAGS) -o $@ $<

clean: removeObjectFiles removeComFiles

removeObjectFiles:
	rm -f *.o $(PROG)

removeComFiles:
	rm -f from*to* Node*received
