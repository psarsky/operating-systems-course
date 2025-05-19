CC = gcc
CFLAGS = -Wall -pthread
LIBS = -pthread

all: syncprinters

syncprinters: sync-printers.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

test: syncprinters
	./syncprinters

clean:
	rm -f syncprinters *.o

.PHONY: all clean test