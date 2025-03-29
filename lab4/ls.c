#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int GLOBAL = 0;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    printf("Program name: %s\n", argv[0]);
    int local = 0;
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        printf("Child process\n");
        GLOBAL++;
        local++;
        printf("child pid = %d, parent pid = %d\n", getpid(), getppid());
        printf("child's local = %d, child's global = %d\n", local, GLOBAL);
        execl("/bin/ls", "ls", argv[1], NULL);
        perror("execl");
        exit(1);
    } else {
        int status;
        wait(&status);
        printf("parent process\n");
        printf("parent pid = %d, child pid = %d\n", getpid(), pid);
        printf("child exit code: %d\n", WEXITSTATUS(status));
        printf("parent's local = %d, parent's global = %d\n", local, GLOBAL);
        return status;
    }
    return 0;
}
