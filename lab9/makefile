CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -lm

all: integral

integral: integral.c
	$(CC) $(CFLAGS) -o integral integral.c $(LDFLAGS)

test: integral
	@time ./integral 0.0000000001 4
	@time ./integral 0.0000000001 8

clean:
	rm -f integral

.PHONY: all clean test