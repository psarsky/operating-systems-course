#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

double RANGE_START = 0.0;
double RANGE_STOP = 1.0;

double f(double x){
    return 4 / (x*x + 1);
}

double calculate_integral(double interval_start, double interval_stop, double (*fun)(double), double interval_width, unsigned long long number_of_intervals){
    if (interval_stop - interval_start < interval_width)
        return fun((interval_start + interval_stop)/ 2.0)*(interval_stop - interval_start);

    double sum = 0.0;
    for (unsigned long long i = 0; i < number_of_intervals; i++){
        sum += fun(interval_start + interval_width/2.0);
        interval_start += interval_width;
    }

    return sum * interval_width;
}

int main(int argc, char** argv) {
    double interval_width = strtod(argv[1], NULL);
    long n = strtol(argv[2], NULL, 10);
    

    if (ceil((RANGE_STOP - RANGE_START)/interval_width) < n) {
        printf("Too many processes for given interval range");
        return -1;
    }

    unsigned long long total_intervals_count = (unsigned long long)ceil((double)(RANGE_STOP - RANGE_START)/interval_width);

    for (int k = 1; k <= n; k++) {
        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        unsigned long long intervals_per_process = total_intervals_count/k;
        double interval_start = RANGE_START;
        double interval_stop = RANGE_START;
        int pipes_fd[k][2];

        for (int i = 0; i < k; i++) {
            if (RANGE_STOP < interval_start + intervals_per_process*interval_width){
                interval_stop = RANGE_STOP;
            } else {
                interval_stop = interval_start + intervals_per_process*interval_width;
            }

            pipe(pipes_fd[i]);
            pid_t pid = fork();
            if(pid == 0){
                close(pipes_fd[i][0]);
                double integral_result = calculate_integral(interval_start, interval_stop, f, interval_width, intervals_per_process);
                write(pipes_fd[i][1], &integral_result, sizeof(integral_result));

                exit(0);
            }
            close(pipes_fd[i][1]);

            interval_start = interval_stop;
        }

        double sum = 0.0;
        for(int i = 0; i < k; i++){
            double integral_result;
            read(pipes_fd[i][0], &integral_result, sizeof(integral_result));
            sum += integral_result;
        }

        clock_gettime(CLOCK_MONOTONIC, &end_time);

        double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

        printf("k = %d; Result: %lf; Time: %.3fs\n", k, sum, elapsed_time);
    }

    return 0;
}