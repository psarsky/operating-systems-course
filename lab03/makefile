CC = gcc

all: flipper test

flipper: flipper.c
	$(CC) flipper.c -o flipper

test: flipper
	./flipper art output

clean:
	rm -f flipper ./output/*
