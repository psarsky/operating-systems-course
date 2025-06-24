#include "collatz.h"
#include <stdio.h>

#define MAX_ITER 1000

int main() {
    int numbers[] = {6, 11, 19, 27, 50};
    int num_tests = sizeof(numbers) / sizeof(int);
    int steps[MAX_ITER];

    printf("Shared linking:\n");
    for (int i = 0; i < num_tests; i++) {
        int num_steps = test_collatz_convergence(numbers[i], MAX_ITER, steps);
        if (num_steps) {
            printf("%d converges to 1 in %d steps: ", numbers[i], num_steps);
            for (int j = 0; j < num_steps; j++) {
                printf("%d ", steps[j]);
            }
            printf("\n");
        } else {
            printf("%d does not converge to 1 in %d iterations.\n", numbers[i], MAX_ITER);
        }
    }

    return 0;
}
