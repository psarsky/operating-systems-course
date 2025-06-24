CC = gcc
CFLAGS = -Wall -Wextra -fPIC
AR = ar
ARFLAGS = rcs

all: collatzlib_static.a collatzlib_shared.so client_static client_shared client_dynamic

# Static library
collatzlib_static.a: collatz.o
	$(AR) $(ARFLAGS) collatzlib_static.a collatz.o

# Shared library
collatzlib_shared.so: collatz.o
	$(CC) -shared -o collatzlib_shared.so collatz.o

# Client with static library
client_static: client_static.c collatzlib_static.a
	$(CC) client_static.c -o client_static collatzlib_static.a

# Client with shared library
client_shared: client_shared.c collatzlib_shared.so
	$(CC) client_shared.c -o client_shared collatzlib_shared.so -Wl,-rpath,.

# Client with dynamic library
client_dynamic: client_dynamic.c
	$(CC) client_dynamic.c -o client_dynamic -ldl

collatz.o: collatz.c collatz.h
	$(CC) $(CFLAGS) -c collatz.c

clean:
	rm -f *.o *.a *.so client_static client_shared client_dynamic
