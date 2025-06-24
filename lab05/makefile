CC = gcc
CFLAGS = -Wall -Wextra -g

all: sigusr1 catcher sender

sigusr1: sigusr1.c
	$(CC) $(CFLAGS) -o sigusr1 sigusr1.c

catcher: catcher.c
	$(CC) $(CFLAGS) -o catcher catcher.c

sender: sender.c
	$(CC) $(CFLAGS) -o sender sender.c

clean:
	rm -f sigusr1 catcher sender