#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

// structure to pass data to threads
typedef struct {
    int thread_id;          // thread identifier
    int num_threads;        // total number of threads
    double rectangle_width; // width of each rectangle
    double *results;        // array to store partial results
    int *ready_flags;       // array to mark completion status
} ThreadData;

// function to calculate
double f(double x) {
    return 4.0 / (x * x + 1.0);
}

// thread function that calculates a portion of the integral
void *calculate_integral(void *arg) {
    // read data for this thread
    ThreadData *data = (ThreadData *)arg;
    int thread_id = data->thread_id;
    int num_threads = data->num_threads;
    double rectangle_width = data->rectangle_width;

    // calculate the start and end points for this thread
    double start_x = (double)thread_id / num_threads;
    double end_x = (thread_id == num_threads - 1) ? 1.0 : (double)(thread_id + 1) / num_threads;

    // calculate the partial sum for this thread
    double sum = 0.0;
    for (double x = start_x; x < end_x; x += rectangle_width) {
        double midpoint = x + rectangle_width / 2.0;
        sum += f(midpoint) * rectangle_width;
    }

    data->results[thread_id] = sum;
    data->ready_flags[thread_id] = 1;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <rectangle_width> <num_threads>\n", argv[0]);
        return 1;
    }

    double rectangle_width = atof(argv[1]);
    int num_threads = atoi(argv[2]);
    
    if (rectangle_width <= 0.0) {
        fprintf(stderr, "Rectangle width must be positive\n");
        return 1;
    }
    
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return 1;
    }
    
    // create arrays for thread results and ready flags
    double *results = calloc(num_threads, sizeof(double));
    int *ready_flags = calloc(num_threads, sizeof(int));
    
    if (!results || !ready_flags) {
        fprintf(stderr, "Memory allocation failed\n");
        free(results);
        free(ready_flags);
        return 1;
    }
    
    // create thread data structures
    ThreadData *thread_data = calloc(num_threads, sizeof(ThreadData));
    pthread_t *threads = calloc(num_threads, sizeof(pthread_t));
    
    if (!thread_data || !threads) {
        fprintf(stderr, "Memory allocation failed\n");
        free(results);
        free(ready_flags);
        free(thread_data);
        free(threads);
        return 1;
    }
    
    // initialize thread data and create threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].rectangle_width = rectangle_width;
        thread_data[i].results = results;
        thread_data[i].ready_flags = ready_flags;
        
        int rc = pthread_create(&threads[i], NULL, calculate_integral, &thread_data[i]);
        if (rc) {
            fprintf(stderr, "Error creating thread %d: %d\n", i, rc);
            return 1;
        }
    }

    // wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    double final_result = 0.0;
    for (int i = 0; i < num_threads; i++) {
        final_result += results[i];
    }

    printf("Width: %e, threads: %d\n", rectangle_width, num_threads);
    printf("Result: %.6f\n", final_result);

    free(results);
    free(ready_flags);
    free(thread_data);
    free(threads);

    return 0;
}