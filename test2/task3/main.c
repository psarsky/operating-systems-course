#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

void* hello(void* arg) {
    sleep(1);
    while(1) {
        printf("Hello world from thread number %d\n", *(int*)arg);
        printf("Hello world again from thread number %d\n", *(int*)arg);
        sleep(2);
    }
    return NULL;
}

int main(int argc, char** args) {

    if(argc !=3) {
        printf("Not a suitable number of program parameters\n");
        return(1);
    }

    int n = atoi(args[1]);
    int main_loop_iterations = atoi(args[2]);

    if (n <= 0 || main_loop_iterations < 0) {
        printf("Invalid parameters\n");
        return 1;
    }

    pthread_t threads[n];
    int thread_args[n];

    for (int i = 0; i < n; ++i) {
        thread_args[i] = i;
        if (pthread_create(&threads[i], NULL, hello, &thread_args[i]) != 0) {
            perror("Error creating thread");
            return 1;
        }
    }

    int i = 0;
    while(i++ < main_loop_iterations) {
        printf("Hello from main %d\n",i);
        sleep(2);
    }

    for (int j = 0; j < n; ++j) {
        pthread_cancel(threads[j]);
    }

    for (int j = 0; j < n; ++j) {
        pthread_join(threads[j], NULL);
    }

    return 0;
}