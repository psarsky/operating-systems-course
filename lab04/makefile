CC = gcc
BUILD_DIR = build
CFLAGS = -Wall

all: child_processes ls tests

child_processes: child_processes.c
	$(CC) $(CFLAGS) -o $@ $<

ls: ls.c
	$(CC) $(CFLAGS) -o $@ $<

tests: 
	./child_processes 10
	./ls .

clean:
	rm -f child_processes ls