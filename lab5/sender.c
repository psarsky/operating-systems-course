#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

bool confirmation_received = false;

void handler() {
    confirmation_received = true;
    printf("Received response from catcher. \n");
}

int main(int argc, char *argv[]) {
    (void)argc;

    pid_t catcher_pid = atoi(argv[1]);
    int mode = atoi(argv[2]);

    printf("Sender PID: %d\n", getpid());

    signal(SIGUSR1, handler);

    union sigval value;
    value.sival_int = mode;

    printf("Sending signal to PID %d using mode %d\n", catcher_pid, mode);
    sigqueue(catcher_pid, SIGUSR1, value);

    while (!confirmation_received) {
        pause();
    }

    return 0;
}