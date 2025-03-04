CC = gcc
CFLAGS = -Wall

.PHONY: all countdown clean

all: countdown

countdown: countdown.c
	$(CC) $(CFLAGS) -o countdown countdown.c

clean:
	rm -f countdown