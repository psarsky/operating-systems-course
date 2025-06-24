#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_processes>\n", argv[0]);
        return 1;
    }
    int num_processes = atoi(argv[1]);
    if (num_processes <= 0) {
        fprintf(stderr, "Number of processes must be positive.\n");
        return 1;
    }
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("PPID: %d, PID: %d\n", getppid(), getpid());
            exit(0);
        }
        if (pid < 0) {
            perror("fork");
            return 1;
        }
    }
    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }
    printf("Number of processes: %d\n", num_processes);
    return 0;
}