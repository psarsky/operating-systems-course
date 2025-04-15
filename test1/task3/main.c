#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void sighandler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGUSR1) {
        printf("Received SIGUSR1 signal with value: %d\n", info->si_value.sival_int);
    }
}

int main(int argc, char* argv[]) {

    if(argc != 3){
        printf("Not a suitable number of program parameters\n");
        return 1;
    }

    struct sigaction action;
    action.sa_sigaction = &sighandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGUSR1, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    int child = fork();
    if(child == 0) {
        sigset_t mask;
        sigfillset(&mask);
        sigdelset(&mask, SIGUSR1);
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
            perror("sigprocmask");
            return 1;
        }
        pause();
        exit(EXIT_SUCCESS);

    }
    else {
        int value = atoi(argv[1]);
        int signum = atoi(argv[2]);

        union sigval val;
        val.sival_int = value;

        if (sigqueue(child, signum, val) == -1) {
            perror("sigqueue");
            return 1;
        }
    }

    return 0;
}
