#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#define N 3             // users
#define M 2             // printers
#define QUEUE_SIZE 10   // max print jobs in queue

// print job structure
typedef struct {
    char text[10];
} print_job;

// print queue structure
typedef struct {
    print_job jobs[QUEUE_SIZE];
    int head;
    int tail;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} print_queue;

print_queue *queue;

void *user(void *arg) {
    // generate a print job
    print_job job;
    for (int i = 0; i < 10; i++) {
        job.text[i] = 'a' + rand() % 26;
    }

    // add the job to the queue
    sem_wait(&queue->slots);    // wait for an empty slot
    sem_wait(&queue->mutex);    // lock the queue

    queue->jobs[queue->tail] = job;                 // add job to the queue
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;   // update tail index

    sem_post(&queue->mutex);    // unlock the queue
    sem_post(&queue->items);    // signal that there is a new job

    sleep(rand() % 5);

    return NULL;
}

void *printer(void *arg) {
    while (1) {
        // get a job from the queue
        sem_wait(&queue->items);    // wait for a job to appear in queue
        sem_wait(&queue->mutex);    // lock the queue

        print_job job = queue->jobs[queue->head];       // get the print job from queue head
        queue->head = (queue->head + 1) % QUEUE_SIZE;   // update head index

        sem_post(&queue->mutex);    // unlock the queue
        sem_post(&queue->slots);    // signal that there is a new empty slot

        // print the job
        for (int i = 0; i < 10; i++) {
            putchar(job.text[i]);
            fflush(stdout);
            sleep(1);
        }
        putchar('\n');

        // if queue is empty, break the loop
        if (queue->head == queue->tail) {
            break;
        }
    }
    return NULL;
}

int main() {
    // thread identifiers
    pthread_t users[N], printers[M];

    // initialize the print queue
    queue = mmap(NULL, sizeof(print_queue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    sem_init(&queue->mutex, 1, 1);          // queue access (one thread at a time)
    sem_init(&queue->slots, 1, QUEUE_SIZE); // empty slots in the queue
    sem_init(&queue->items, 1, 0);          // items in the queue

    queue->head = 0;    // index of the first job
    queue->tail = 0;    // index of the last job

    // create the user and printer threads
    for (int i = 0; i < N; i++) {
        pthread_create(&users[i], NULL, user, NULL);        // N user threads
    }
    for (int i = 0; i < M; i++) {
        pthread_create(&printers[i], NULL, printer, NULL);  // M printer threads
    }

    // wait for all threads to finish
    for (int i = 0; i < N; i++) {
        pthread_join(users[i], NULL);
    }
    for (int i = 0; i < M; i++) {
        pthread_join(printers[i], NULL);
    }

    return 0;
}