#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

void handler() {
    printf("Received SIGUSR1 signal in handler.\n");
}

int main(int argc, char *argv[]) {
    (void)argc;
    sigset_t sigset;

    if (strcmp(argv[1], "ignore") == 0) {
        signal(SIGUSR1, SIG_IGN);
    } else if (strcmp(argv[1], "handler") == 0) {
        signal(SIGUSR1, handler);
    } else if (strcmp(argv[1], "mask") == 0) {
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sigset, NULL);
    } else if (strcmp(argv[1], "none") == 0) {
        signal(SIGUSR1, SIG_DFL);
    } else {
        fprintf(stderr, "Unknown param: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    raise(SIGUSR1);

    sigpending(&sigset);

    if (sigismember(&sigset, SIGUSR1)) {
        printf("SIGUSR1 signal pending.\n");
    } else {
        printf("No pending SIGUSR1 signal.\n");
    }

    return 0;
}