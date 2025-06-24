CC = gcc
CFLAGS = -Wall -std=c17

.PHONY: all countdown clean

all: countdown

countdown: countdown.c
	$(CC) $(CFLAGS) -o countdown countdown.c

countdowndebug: countdown.c
	$(CC) $(CFLAGS) -g -o countdown countdown.c

clean:
	rm -f countdown