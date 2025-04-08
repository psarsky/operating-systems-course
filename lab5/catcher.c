#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

int requests = 0;
int mode = 1;
int sender_pid = 0;

void sigint_handler(int sig) {
    (void)sig;
    printf("CTRL+C\n");
}

void handler(int sig, siginfo_t *info, void *ucontext) {
    (void)ucontext;
    sender_pid = info->si_pid;
    mode = info->si_value.sival_int;

    printf("Signal %d received from process: %d with value: %d\n", sig, info->si_pid, info->si_value.sival_int);

    requests++;

    struct sigaction sa_int;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (mode == 3) {
        sa_int.sa_handler = SIG_IGN;
    } else if (mode == 4) {
        sa_int.sa_handler = sigint_handler;
    } else {
        sa_int.sa_handler = SIG_DFL;
    }

    sigaction(SIGINT, &sa_int, NULL);

    kill(sender_pid, SIGUSR1);
}

int main() {
    printf("PID: %d\n", getpid());

    struct sigaction sa_usr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_SIGINFO;
    sa_usr1.sa_sigaction = handler;

    sigaction(SIGUSR1, &sa_usr1, NULL);

    while(1) {
        if (sender_pid != 0) { 
            kill(sender_pid, SIGUSR1);
            switch (mode) {
                case 1:
                    printf("Mode change requests: %d\n", requests);
                    break;
                case 2:
                    for (int i = 1; mode == 2; i++) {
                        printf("%d\n", i);
                        sleep(1);
                    }
                    break;
                case 3:
                    printf("Ignoring Ctrl+C.\n");
                    break;
                case 4:
                    printf("Ctrl+C prints a message.\n");
                    break;
                case 5:
                    printf("Closing catcher.\n");
                    exit(0);
                    break;
            }
            mode = 0;
            sender_pid = 0;
        }
    }
    return 0;
}